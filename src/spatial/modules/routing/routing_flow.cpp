#include "spatial/modules/routing/routing_module.hpp"
#include "spatial/modules/routing/routing_graph.hpp"

#include "duckdb/common/exception.hpp"
#include "duckdb/common/operator/add.hpp"
#include "duckdb/common/string_util.hpp"

#include <algorithm>

namespace duckdb {

namespace routing {

namespace {

//----------------------------------------------------------------------------------------------------------------------
// Flow network
//----------------------------------------------------------------------------------------------------------------------
class FlowNetwork {
public:
	struct FlowArc {
		uint32_t head;
		uint32_t next;
		int64_t capacity;
		double cost;
	};

	explicit FlowNetwork(uint32_t vertex_count) : first(vertex_count, INVALID_ARC) {
	}

	//! Adds the arc and its residual twin, returns the index of the arc
	uint32_t AddArc(uint32_t from, uint32_t to, int64_t capacity, double cost) {
		const auto index = UnsafeNumericCast<uint32_t>(arcs.size());
		arcs.push_back(FlowArc {to, first[from], capacity, cost});
		first[from] = index;
		arcs.push_back(FlowArc {from, first[to], 0, -cost});
		first[to] = index + 1;
		return index;
	}

	uint32_t VertexCount() const {
		return UnsafeNumericCast<uint32_t>(first.size());
	}

	//! Dinic's algorithm
	int64_t MaxFlow(ClientContext &context, uint32_t source, uint32_t sink) {
		int64_t total = 0;
		vector<uint32_t> level(first.size());
		vector<uint32_t> cursor(first.size());
		vector<uint32_t> queue;
		vector<uint32_t> path;
		while (true) {
			CheckInterrupt(context);
			std::fill(level.begin(), level.end(), INVALID_VERTEX);
			queue.clear();
			queue.push_back(source);
			level[source] = 0;
			for (idx_t i = 0; i < queue.size(); i++) {
				const auto vertex = queue[i];
				for (auto arc = first[vertex]; arc != INVALID_ARC; arc = arcs[arc].next) {
					if (arcs[arc].capacity > 0 && level[arcs[arc].head] == INVALID_VERTEX) {
						level[arcs[arc].head] = level[vertex] + 1;
						queue.push_back(arcs[arc].head);
					}
				}
			}
			if (level[sink] == INVALID_VERTEX) {
				return total;
			}
			cursor = first;
			path.clear();
			auto vertex = source;
			while (true) {
				if (vertex == sink) {
					auto pushed = NumericLimits<int64_t>::Maximum();
					for (auto arc : path) {
						pushed = MinValue(pushed, arcs[arc].capacity);
					}
					for (auto arc : path) {
						arcs[arc].capacity -= pushed;
						arcs[arc ^ 1].capacity += pushed;
					}
					total += pushed;
					path.clear();
					vertex = source;
					continue;
				}
				auto &arc = cursor[vertex];
				while (arc != INVALID_ARC &&
				       !(arcs[arc].capacity > 0 && level[arcs[arc].head] == level[vertex] + 1)) {
					arc = arcs[arc].next;
				}
				if (arc != INVALID_ARC) {
					path.push_back(arc);
					vertex = arcs[arc].head;
				} else if (path.empty()) {
					break;
				} else {
					level[vertex] = INVALID_VERTEX;
					vertex = arcs[path.back() ^ 1].head;
					path.pop_back();
				}
			}
		}
	}

	//! Successive shortest paths with node potentials. Costs have to be non-negative.
	int64_t MinCostMaxFlow(ClientContext &context, uint32_t source, uint32_t sink) {
		int64_t total = 0;
		const auto count = first.size();
		vector<double> potential(count, 0);
		vector<double> dist(count);
		vector<uint32_t> pred(count);
		MinHeap heap;
		while (true) {
			CheckInterrupt(context);
			std::fill(dist.begin(), dist.end(), Infinity());
			std::fill(pred.begin(), pred.end(), INVALID_ARC);
			heap.Clear();
			dist[source] = 0;
			heap.Push(0, source);
			while (!heap.Empty()) {
				const auto entry = heap.Pop();
				if (entry.key > dist[entry.vertex]) {
					continue;
				}
				for (auto arc = first[entry.vertex]; arc != INVALID_ARC; arc = arcs[arc].next) {
					if (arcs[arc].capacity <= 0) {
						continue;
					}
					const auto head = arcs[arc].head;
					// Reduced costs are non-negative up to rounding
					const auto reduced = MaxValue(0.0, arcs[arc].cost + potential[entry.vertex] - potential[head]);
					const auto candidate = entry.key + reduced;
					if (candidate < dist[head]) {
						dist[head] = candidate;
						pred[head] = arc;
						heap.Push(candidate, head);
					}
				}
			}
			if (dist[sink] == Infinity()) {
				return total;
			}
			for (idx_t v = 0; v < count; v++) {
				if (dist[v] != Infinity()) {
					potential[v] += dist[v];
				}
			}
			auto pushed = NumericLimits<int64_t>::Maximum();
			for (auto v = sink; v != source; v = arcs[pred[v] ^ 1].head) {
				pushed = MinValue(pushed, arcs[pred[v]].capacity);
			}
			for (auto v = sink; v != source; v = arcs[pred[v] ^ 1].head) {
				arcs[pred[v]].capacity -= pushed;
				arcs[pred[v] ^ 1].capacity += pushed;
			}
			total += pushed;
		}
	}

	vector<FlowArc> arcs;

private:
	vector<uint32_t> first;
};

//----------------------------------------------------------------------------------------------------------------------
// pgr_maxFlow, pgr_pushRelabel, pgr_edmondsKarp, pgr_boykovKolmogorov, pgr_maxFlowMinCost
//----------------------------------------------------------------------------------------------------------------------
enum class FlowOutput : uint8_t { TOTAL, EDGES, MIN_COST };

struct FlowData : public RoutingBindData {
	FlowOutput output = FlowOutput::TOTAL;
	vector<int64_t> source_vids;
	vector<int64_t> sink_vids;
	bool empty = false;

	struct FlowEdge {
		int64_t id;
		int64_t source;
		int64_t target;
		int64_t capacity;
		int64_t reverse_capacity;
		double cost;
		double reverse_cost;
	};

	struct FlowRow {
		int64_t edge;
		int64_t from;
		int64_t to;
		int64_t flow;
		int64_t residual;
		double cost;
	};

	void Compute(ClientContext &context, const RoutingInput &input, RoutingResult &result) const override {
		if (empty) {
			return;
		}
		const auto min_cost = output == FlowOutput::MIN_COST;
		vector<FlowEdge> edges;
		edges.reserve(input.row_count);
		vector<int64_t> vertex_ids;
		int64_t capacity_sum = 0;
		for (idx_t i = 0; i < input.row_count; i++) {
			FlowEdge edge;
			edge.id = input.Get(0).integers[i];
			edge.source = input.Get(1).integers[i];
			edge.target = input.Get(2).integers[i];
			edge.capacity = input.Get(3).integers[i];
			edge.reverse_capacity = input.Has(4) ? input.Get(4).integers[i] : -1;
			edge.cost = min_cost ? input.Get(5).numerics[i] : 0;
			edge.reverse_cost = min_cost && input.Has(6) ? input.Get(6).numerics[i] : 0;
			if (min_cost && ((edge.capacity > 0 && !(edge.cost >= 0)) ||
			                 (edge.reverse_capacity > 0 && input.Has(6) && !(edge.reverse_cost >= 0)))) {
				throw InvalidInputException("%s: edge %lld has a negative cost", function_name, edge.id);
			}
			if (min_cost && edge.reverse_capacity > 0 && !input.Has(6)) {
				throw InvalidInputException(
				    "%s: the column \"reverse_cost\" is required when \"reverse_capacity\" is used", function_name);
			}
			for (auto capacity : {edge.capacity, edge.reverse_capacity}) {
				if (capacity > 0 && !TryAddOperator::Operation(capacity_sum, capacity, capacity_sum)) {
					throw InvalidInputException("%s: the sum of the capacities overflows a BIGINT", function_name);
				}
			}
			edges.push_back(edge);
			vertex_ids.push_back(edge.source);
			vertex_ids.push_back(edge.target);
		}
		std::sort(edges.begin(), edges.end(), [](const FlowEdge &a, const FlowEdge &b) {
			if (a.id != b.id) {
				return a.id < b.id;
			}
			if (a.source != b.source) {
				return a.source < b.source;
			}
			if (a.target != b.target) {
				return a.target < b.target;
			}
			if (a.capacity != b.capacity) {
				return a.capacity < b.capacity;
			}
			if (a.reverse_capacity != b.reverse_capacity) {
				return a.reverse_capacity < b.reverse_capacity;
			}
			if (a.cost != b.cost) {
				return a.cost < b.cost;
			}
			return a.reverse_cost < b.reverse_cost;
		});
		std::sort(vertex_ids.begin(), vertex_ids.end());
		vertex_ids.erase(std::unique(vertex_ids.begin(), vertex_ids.end()), vertex_ids.end());
		if (vertex_ids.size() >= INVALID_VERTEX - 2 || edges.size() >= INVALID_ARC / 8) {
			throw InvalidInputException("%s: the graph is too large", function_name);
		}

		auto lookup = [&](int64_t id, uint32_t &index) {
			auto entry = std::lower_bound(vertex_ids.begin(), vertex_ids.end(), id);
			if (entry == vertex_ids.end() || *entry != id) {
				return false;
			}
			index = UnsafeNumericCast<uint32_t>(entry - vertex_ids.begin());
			return true;
		};

		vector<uint32_t> sources;
		vector<uint32_t> sinks;
		uint32_t index = 0;
		for (auto id : source_vids) {
			if (lookup(id, index)) {
				sources.push_back(index);
			}
		}
		for (auto id : sink_vids) {
			if (lookup(id, index)) {
				sinks.push_back(index);
			}
		}

		const auto vertex_count = UnsafeNumericCast<uint32_t>(vertex_ids.size());
		const auto super_source = vertex_count;
		const auto super_sink = vertex_count + 1;
		FlowNetwork network(vertex_count + 2);
		struct ArcRef {
			uint32_t arc;
			uint32_t edge;
			bool reverse;
		};
		vector<ArcRef> arc_refs;
		for (idx_t i = 0; i < edges.size(); i++) {
			auto &edge = edges[i];
			uint32_t from = 0;
			uint32_t to = 0;
			lookup(edge.source, from);
			lookup(edge.target, to);
			if (edge.capacity > 0) {
				arc_refs.push_back(
				    ArcRef {network.AddArc(from, to, edge.capacity, edge.cost), UnsafeNumericCast<uint32_t>(i), false});
			}
			if (edge.reverse_capacity > 0) {
				arc_refs.push_back(ArcRef {network.AddArc(to, from, edge.reverse_capacity, edge.reverse_cost),
				                           UnsafeNumericCast<uint32_t>(i), true});
			}
		}
		const auto unbounded = MaxValue<int64_t>(capacity_sum, 1);
		for (auto source : sources) {
			network.AddArc(super_source, source, unbounded, 0);
		}
		for (auto sink : sinks) {
			network.AddArc(sink, super_sink, unbounded, 0);
		}

		int64_t total = 0;
		if (!sources.empty() && !sinks.empty()) {
			total = min_cost ? network.MinCostMaxFlow(context, super_source, super_sink)
			                 : network.MaxFlow(context, super_source, super_sink);
		}

		if (output == FlowOutput::TOTAL) {
			result.BeginRow();
			result.SetInteger(0, total);
			return;
		}

		vector<FlowRow> rows;
		for (auto &ref : arc_refs) {
			auto &edge = edges[ref.edge];
			const auto flow = network.arcs[ref.arc ^ 1].capacity;
			if (flow <= 0) {
				continue;
			}
			FlowRow row;
			row.edge = edge.id;
			row.from = ref.reverse ? edge.target : edge.source;
			row.to = ref.reverse ? edge.source : edge.target;
			row.flow = flow;
			row.residual = network.arcs[ref.arc].capacity;
			row.cost = static_cast<double>(flow) * (ref.reverse ? edge.reverse_cost : edge.cost);
			rows.push_back(row);
		}
		std::sort(rows.begin(), rows.end(), [](const FlowRow &a, const FlowRow &b) {
			if (a.from != b.from) {
				return a.from < b.from;
			}
			if (a.to != b.to) {
				return a.to < b.to;
			}
			return a.edge < b.edge;
		});
		int64_t seq = 0;
		double agg_cost = 0;
		for (auto &row : rows) {
			result.BeginRow();
			result.SetInteger(0, ++seq);
			result.SetInteger(1, row.edge);
			result.SetInteger(2, row.from);
			result.SetInteger(3, row.to);
			result.SetInteger(4, row.flow);
			result.SetInteger(5, row.residual);
			if (min_cost) {
				agg_cost += row.cost;
				result.SetDouble(6, row.cost);
				result.SetDouble(7, agg_cost);
			}
		}
	}
};

template <FlowOutput OUTPUT>
unique_ptr<FunctionData> BindFlow(ClientContext &context, TableFunctionBindInput &input,
                                  vector<LogicalType> &return_types, vector<string> &names) {
	auto result = make_uniq<FlowData>();
	result->output = OUTPUT;
	const auto name = input.table_function.name.c_str();
	RoutingBinder binder(name, input, 3);
	vector<ColumnSpec> specs = {{"id", ColumnKind::INTEGER, true},
	                            {"source", ColumnKind::INTEGER, true},
	                            {"target", ColumnKind::INTEGER, true},
	                            {"capacity", ColumnKind::INTEGER, true},
	                            {"reverse_capacity", ColumnKind::INTEGER, false}};
	if (OUTPUT == FlowOutput::MIN_COST) {
		specs.push_back({"cost", ColumnKind::NUMERIC, true});
		specs.push_back({"reverse_cost", ColumnKind::NUMERIC, false});
	}
	binder.BindColumns(*result, specs, "edges");
	result->source_vids = binder.VertexArgument(1, "start vids");
	result->sink_vids = binder.VertexArgument(2, "end vids");
	binder.Finish();
	result->empty = binder.has_null_argument;
	for (auto id : result->source_vids) {
		if (std::binary_search(result->sink_vids.begin(), result->sink_vids.end(), id)) {
			throw BinderException("%s: vertex %lld is both a source and a sink", name, id);
		}
	}

	switch (OUTPUT) {
	case FlowOutput::TOTAL:
		SetResultSchema(*result, return_types, names, {{"pgr_maxflow", LogicalType::BIGINT}});
		break;
	case FlowOutput::EDGES:
		SetResultSchema(*result, return_types, names,
		                {{"seq", LogicalType::INTEGER},
		                 {"edge", LogicalType::BIGINT},
		                 {"start_vid", LogicalType::BIGINT},
		                 {"end_vid", LogicalType::BIGINT},
		                 {"flow", LogicalType::BIGINT},
		                 {"residual_capacity", LogicalType::BIGINT}});
		break;
	default:
		SetResultSchema(*result, return_types, names,
		                {{"seq", LogicalType::INTEGER},
		                 {"edge", LogicalType::BIGINT},
		                 {"source", LogicalType::BIGINT},
		                 {"target", LogicalType::BIGINT},
		                 {"flow", LogicalType::BIGINT},
		                 {"residual_capacity", LogicalType::BIGINT},
		                 {"cost", LogicalType::DOUBLE},
		                 {"agg_cost", LogicalType::DOUBLE}});
		break;
	}
	return std::move(result);
}

const char *const FLOW_EDGES_DOC = R"(
The edges are a table-valued argument, i.e. a parenthesised subquery, whose columns are matched by name:

| Column | Type | Description |
| --- | --- | --- |
| `id` | integer | Identifier of the edge |
| `source` | integer | Identifier of the first end point vertex |
| `target` | integer | Identifier of the second end point vertex |
| `capacity` | integer | Capacity of the edge (`source`, `target`). A value that is not positive means the edge does not exist in that direction |
| `reverse_capacity` | integer | Optional. Capacity of the edge (`target`, `source`). A value that is not positive, or a missing column, means the edge does not exist in that direction |

`start vids` and `end vids` are a single integer or a list of integers, given as constants. With several sources or sinks the flow goes from any source to any sink. A vertex cannot be on both sides.
)";

const char *const FLOW_RESULT_DOC = R"(
The result has one row per edge direction that carries flow, ordered by `start_vid`, `end_vid` and `edge`:

| Column | Type | Description |
| --- | --- | --- |
| `seq` | INTEGER | Sequential value starting from 1 |
| `edge` | BIGINT | Identifier of the edge |
| `start_vid` | BIGINT | Vertex the flow leaves from |
| `end_vid` | BIGINT | Vertex the flow goes to |
| `flow` | BIGINT | Flow through the edge in that direction |
| `residual_capacity` | BIGINT | Capacity left in that direction |

Differences with pgRouting: the edges are a table-valued argument instead of an SQL string and the combinations signature is not available. `pgr_pushRelabel`, `pgr_edmondsKarp` and `pgr_boykovKolmogorov` are the same function here: the maximum flow is computed with Dinic's algorithm whatever the name. The total flow is the same as with pgRouting, but a maximum flow is generally not unique, so the flow of individual edges may differ.
)";

void RegisterEdgeFlow(ExtensionLoader &loader, const char *name) {
	const auto description = StringUtil::Format(R"(
Maximum flow from the source(s) to the sink(s), with the flow carried by each edge.

`%s(edges, start vids, end vids)`
%s%s)",
	                                            name, FLOW_EDGES_DOC, FLOW_RESULT_DOC);
	const auto example = StringUtil::Format(R"(
SELECT * FROM %s((SELECT id, source, target, capacity, reverse_capacity FROM edges), 11, 12);
)",
	                                        name);
	RegisterRoutingFunction(loader, name, 2, BindFlow<FlowOutput::EDGES>, {}, description.c_str(), example.c_str());
}

} // namespace

void RegisterFlowFunctions(ExtensionLoader &loader) {
	RegisterRoutingFunction(loader, "pgr_maxFlow", 2, BindFlow<FlowOutput::TOTAL>, {},
	                        (string(R"(
Value of the maximum flow from the source(s) to the sink(s), computed with Dinic's algorithm.

`pgr_maxFlow(edges, start vids, end vids)`
)") + FLOW_EDGES_DOC +
	                         R"(
The result is a single row with the column `pgr_maxflow` (BIGINT), which is 0 when no sink can be reached. By the max-flow min-cut theorem it is also the capacity of the minimum cut that separates the sources from the sinks.

Differences with pgRouting: the edges are a table-valued argument instead of an SQL string, the function is a table function that returns one row instead of a scalar, and the combinations signature is not available.
)")
	                            .c_str(),
	                        R"(
SELECT * FROM pgr_maxFlow((SELECT id, source, target, capacity, reverse_capacity FROM edges), 11, 12);
)");

	RegisterEdgeFlow(loader, "pgr_pushRelabel");
	RegisterEdgeFlow(loader, "pgr_edmondsKarp");
	RegisterEdgeFlow(loader, "pgr_boykovKolmogorov");

	for (auto name : {"pgr_maxFlowMinCost", "pgr_minCostMaxFlow"}) {
		const auto description = StringUtil::Format(R"(
Maximum flow of minimum cost from the source(s) to the sink(s): among all the maximum flows, one whose total cost is the smallest.

`%s(edges, start vids, end vids)`
%s
In addition to the columns above the edges need `cost` (numeric): the cost of sending one unit of flow from `source` to `target`, and `reverse_cost` (numeric, required when `reverse_capacity` is used): the cost of one unit from `target` to `source`. The existence of an edge direction is decided by its capacity only, and the cost of a usable direction cannot be negative.

The result has one row per edge direction that carries flow, ordered by `source`, `target` and `edge`:

| Column | Type | Description |
| --- | --- | --- |
| `seq` | INTEGER | Sequential value starting from 1 |
| `edge` | BIGINT | Identifier of the edge |
| `source` | BIGINT | Vertex the flow leaves from |
| `target` | BIGINT | Vertex the flow goes to |
| `flow` | BIGINT | Flow through the edge in that direction |
| `residual_capacity` | BIGINT | Capacity left in that direction |
| `cost` | DOUBLE | Cost of the flow through the edge: `flow` times the unit cost |
| `agg_cost` | DOUBLE | Aggregate cost up to this row, the last row holds the total cost |

The flow is computed with the successive shortest path algorithm, which runs one shortest path search per augmentation.

Differences with pgRouting: the edges are a table-valued argument instead of an SQL string and the combinations signature is not available. `pgr_maxFlowMinCost` is the pgRouting name, `pgr_minCostMaxFlow` is an alias. The total flow and the total cost are the same as with pgRouting, but the optimal flow is generally not unique, so the flow of individual edges may differ.
)",
		                                            name, FLOW_EDGES_DOC);
		const auto example = StringUtil::Format(R"(
SELECT * FROM %s((SELECT id, source, target, capacity, reverse_capacity, cost, reverse_cost FROM edges), 11, 12);
)",
		                                        name);
		RegisterRoutingFunction(loader, name, 2, BindFlow<FlowOutput::MIN_COST>, {}, description.c_str(),
		                        example.c_str());
	}
}

} // namespace routing

} // namespace duckdb
