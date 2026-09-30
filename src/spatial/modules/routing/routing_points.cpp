#include "spatial/modules/routing/routing_module.hpp"
#include "spatial/modules/routing/routing_search.hpp"

#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <unordered_map>

namespace duckdb {

namespace routing {

namespace {

//----------------------------------------------------------------------------------------------------------------------
// pgr_withPoints
//----------------------------------------------------------------------------------------------------------------------
struct PointRecord {
	int64_t pid;
	int64_t edge_id;
	double fraction;
	char side;
};

struct WithPointsData : public RoutingBindData {
	vector<PointRecord> points;
	vector<int64_t> start_vids;
	vector<int64_t> end_vids;
	char driving_side = 'r';
	bool directed = true;
	bool details = false;
	bool empty = false;

	static void AddChain(vector<EdgeRecord> &records, const EdgeRecord &edge, const vector<PointRecord> &points,
	                     bool use_cost, bool use_reverse_cost) {
		auto previous = edge.source;
		double previous_fraction = 0;
		for (idx_t i = 0; i <= points.size(); i++) {
			const auto last = i == points.size();
			const auto next = last ? edge.target : -points[i].pid;
			const auto fraction = last ? 1.0 : points[i].fraction;
			const auto delta = fraction - previous_fraction;
			EdgeRecord record;
			record.id = edge.id;
			record.source = previous;
			record.target = next;
			record.cost = use_cost && edge.cost >= 0 ? edge.cost * delta : -1;
			record.reverse_cost = use_reverse_cost && edge.reverse_cost >= 0 ? edge.reverse_cost * delta : -1;
			records.push_back(record);
			previous = next;
			previous_fraction = fraction;
		}
	}

	vector<EdgeRecord> SplitEdges(vector<EdgeRecord> edges) const {
		vector<EdgeRecord> records;
		records.reserve(edges.size() + points.size() * 2);
		vector<PointRecord> forward;
		vector<PointRecord> backward;
		const auto side = directed ? driving_side : 'b';

		auto point = points.begin();
		for (auto &edge : edges) {
			while (point != points.end() && point->edge_id < edge.id) {
				point++;
			}
			if (point == points.end() || point->edge_id != edge.id) {
				records.push_back(edge);
				continue;
			}
			forward.clear();
			backward.clear();
			// As in pgRouting, the side only matters on edges that can be travelled in both directions
			const auto one_way = !(edge.cost >= 0) || !(edge.reverse_cost >= 0);
			for (auto entry = point; entry != points.end() && entry->edge_id == edge.id; entry++) {
				if (side == 'b' || entry->side == 'b' || one_way) {
					forward.push_back(*entry);
					backward.push_back(*entry);
				} else if (entry->side == side) {
					forward.push_back(*entry);
				} else {
					backward.push_back(*entry);
				}
			}
			if (side == 'b') {
				AddChain(records, edge, forward, true, true);
			} else {
				AddChain(records, edge, forward, true, false);
				AddChain(records, edge, backward, false, true);
			}
		}
		SortEdges(records);
		return records;
	}

	// Same rule as pgRouting: consecutive stops on the same edge are merged into the first one
	static void EliminateDetails(Path &path) {
		vector<PathStop> stops;
		for (auto &stop : path.stops) {
			if (!stops.empty() && stops.back().edge == stop.edge) {
				stops.back().cost += stop.cost;
			} else {
				stops.push_back(stop);
			}
		}
		path.stops = std::move(stops);
		FinishPath(path);
	}

	void Compute(ClientContext &context, const RoutingInput &input, RoutingResult &result) const override {
		if (empty) {
			return;
		}
		Graph graph(SplitEdges(ReadEdges(input, EDGE_COLUMN_INDEXES)), directed);
		auto paths = ShortestPaths(context, graph, start_vids, end_vids);
		if (!details) {
			for (auto &path : paths) {
				EliminateDetails(path);
			}
		}
		EmitPaths(result, paths);
	}
};

vector<PointRecord> ParsePoints(RoutingBinder &binder, idx_t argument) {
	vector<PointRecord> points;
	auto list = ParseStructList(binder, argument, "points");
	if (list.names.empty()) {
		return points;
	}
	idx_t pid_idx = 0;
	idx_t edge_idx = 0;
	idx_t fraction_idx = 0;
	idx_t side_idx = 0;
	const auto has_pid = list.Find("pid", pid_idx);
	const auto has_side = list.Find("side", side_idx);
	if (!list.Find("edge_id", edge_idx) || !list.Find("fraction", fraction_idx)) {
		throw BinderException("%s: the points need the fields \"edge_id\" and \"fraction\"", binder.name);
	}
	if ((has_pid && !list.types[pid_idx].IsIntegral()) || !list.types[edge_idx].IsIntegral()) {
		throw BinderException("%s: the fields \"pid\" and \"edge_id\" of the points must be integers", binder.name);
	}
	if (!list.types[fraction_idx].IsNumeric()) {
		throw BinderException("%s: the field \"fraction\" of the points must be numeric", binder.name);
	}
	int64_t next_pid = 0;
	for (auto &row : list.rows) {
		PointRecord point;
		next_pid++;
		if ((has_pid && row[pid_idx].IsNull()) || row[edge_idx].IsNull() || row[fraction_idx].IsNull()) {
			throw BinderException("%s: unexpected NULL value in the points", binder.name);
		}
		point.pid = has_pid ? row[pid_idx].DefaultCastAs(LogicalType::BIGINT).GetValue<int64_t>() : next_pid;
		point.edge_id = row[edge_idx].DefaultCastAs(LogicalType::BIGINT).GetValue<int64_t>();
		point.fraction = row[fraction_idx].DefaultCastAs(LogicalType::DOUBLE).GetValue<double>();
		point.side = 'b';
		if (has_side && !row[side_idx].IsNull()) {
			const auto side = StringUtil::Lower(row[side_idx].ToString());
			if (side != "r" && side != "l" && side != "b") {
				throw BinderException("%s: invalid point side '%s', valid values are 'r', 'l' and 'b'", binder.name,
				                      row[side_idx].ToString());
			}
			point.side = side[0];
		}
		if (!(point.fraction >= 0 && point.fraction <= 1)) {
			throw BinderException("%s: the fraction of point %lld must be between 0 and 1", binder.name, point.pid);
		}
		points.push_back(point);
	}
	std::sort(points.begin(), points.end(), [](const PointRecord &a, const PointRecord &b) {
		if (a.pid != b.pid) {
			return a.pid < b.pid;
		}
		if (a.edge_id != b.edge_id) {
			return a.edge_id < b.edge_id;
		}
		if (a.fraction != b.fraction) {
			return a.fraction < b.fraction;
		}
		return a.side < b.side;
	});
	for (idx_t i = 1; i < points.size();) {
		auto &a = points[i - 1];
		auto &b = points[i];
		if (a.pid != b.pid) {
			i++;
			continue;
		}
		if (a.edge_id != b.edge_id || a.fraction != b.fraction || a.side != b.side) {
			throw BinderException("%s: point %lld is given with different edge, fraction or side values", binder.name,
			                      a.pid);
		}
		points.erase(points.begin() + NumericCast<int64_t>(i));
	}
	std::sort(points.begin(), points.end(), [](const PointRecord &a, const PointRecord &b) {
		if (a.edge_id != b.edge_id) {
			return a.edge_id < b.edge_id;
		}
		if (a.fraction != b.fraction) {
			return a.fraction < b.fraction;
		}
		return a.pid < b.pid;
	});
	return points;
}

unique_ptr<FunctionData> BindWithPoints(ClientContext &context, TableFunctionBindInput &input,
                                        vector<LogicalType> &return_types, vector<string> &names) {
	auto result = make_uniq<WithPointsData>();
	RoutingBinder binder("pgr_withPoints", input, 4);
	binder.BindColumns(*result, EDGE_COLUMN_SPECS, "edges");
	result->points = ParsePoints(binder, 1);
	result->start_vids = binder.VertexArgument(2, "start vids");
	result->end_vids = binder.VertexArgument(3, "end vids");
	const auto side = binder.Option("driving_side", LogicalType::VARCHAR, Value("")).ToString();
	result->directed = binder.Option("directed", LogicalType::BOOLEAN, Value::BOOLEAN(true)).GetValue<bool>();
	result->details = binder.Option("details", LogicalType::BOOLEAN, Value::BOOLEAN(false)).GetValue<bool>();
	binder.Finish();
	result->empty = binder.has_null_argument;

	const auto lower_side = StringUtil::Lower(side);
	if (lower_side.empty()) {
		result->driving_side = result->directed ? 'r' : 'b';
	} else if (lower_side == "r" || lower_side == "l" || lower_side == "b") {
		result->driving_side = lower_side[0];
	} else {
		throw BinderException("pgr_withPoints: invalid driving side '%s', valid values are 'r', 'l' and 'b'", side);
	}
	SetResultSchema(*result, return_types, names, PathSchema());
	return std::move(result);
}

//----------------------------------------------------------------------------------------------------------------------
// pgr_trsp
//----------------------------------------------------------------------------------------------------------------------
struct Restriction {
	vector<int64_t> path;
	double cost;
};

//! Aho-Corasick automaton over edge identifiers. A state is the longest suffix of the travelled edge sequence that is
//! a prefix of a restriction, which is all the history needed to know when a restriction gets completed.
class RestrictionAutomaton {
public:
	explicit RestrictionAutomaton(const vector<Restriction> &restrictions) {
		nodes.emplace_back();
		for (auto &restriction : restrictions) {
			uint32_t state = 0;
			for (auto edge : restriction.path) {
				auto entry = nodes[state].children.find(edge);
				if (entry == nodes[state].children.end()) {
					const auto next = UnsafeNumericCast<uint32_t>(nodes.size());
					nodes[state].children[edge] = next;
					nodes.emplace_back();
					state = next;
				} else {
					state = entry->second;
				}
			}
			nodes[state].penalty += restriction.cost;
		}
		vector<uint32_t> queue;
		for (auto &child : nodes[0].children) {
			queue.push_back(child.second);
		}
		for (idx_t i = 0; i < queue.size(); i++) {
			const auto state = queue[i];
			for (auto &child : nodes[state].children) {
				nodes[child.second].fail = Next(nodes[state].fail, child.first);
				nodes[child.second].penalty += nodes[nodes[child.second].fail].penalty;
				queue.push_back(child.second);
			}
		}
	}

	uint32_t Next(uint32_t state, int64_t edge) const {
		while (true) {
			auto entry = nodes[state].children.find(edge);
			if (entry != nodes[state].children.end()) {
				return entry->second;
			}
			if (state == 0) {
				return 0;
			}
			state = nodes[state].fail;
		}
	}

	double Penalty(uint32_t state) const {
		return nodes[state].penalty;
	}

	idx_t StateCount() const {
		return nodes.size();
	}

private:
	struct Node {
		std::map<int64_t, uint32_t> children;
		uint32_t fail = 0;
		double penalty = 0;
	};
	vector<Node> nodes;
};

struct TurnRestrictedData : public RoutingBindData {
	vector<Restriction> restrictions;
	vector<int64_t> start_vids;
	vector<int64_t> end_vids;
	bool directed = true;
	bool empty = false;

	struct Label {
		uint32_t vertex;
		uint32_t state;
		double dist;
		uint32_t pred_label;
		uint32_t arc;
		double cost;
	};

	//! Labels are (arriving arc, automaton state) pairs: the arc is needed to refuse a U-turn on the edge that was
	//! just travelled, which would otherwise be the cheapest way around most restrictions.
	void Search(ClientContext &context, const Graph &graph, const RestrictionAutomaton &automaton, uint32_t source,
	            const vector<uint32_t> &targets, const vector<uint8_t> &target_flags, vector<Path> &paths) const {
		const auto state_count = automaton.StateCount();
		vector<Label> labels;
		std::unordered_map<uint64_t, uint32_t> label_index;
		vector<uint32_t> target_label(graph.VertexCount(), INVALID_VERTEX);
		MinHeap heap;

		labels.push_back(Label {source, 0, 0, INVALID_VERTEX, INVALID_ARC, 0});
		heap.Push(0, 0);
		auto remaining = targets.size();

		idx_t iterations = 0;
		while (!heap.Empty()) {
			if ((++iterations & 0xFFFF) == 0) {
				CheckInterrupt(context);
			}
			const auto entry = heap.Pop();
			const auto label_idx = entry.vertex;
			const auto label = labels[label_idx];
			if (entry.key > label.dist) {
				continue;
			}
			if (target_flags[label.vertex] && target_label[label.vertex] == INVALID_VERTEX) {
				target_label[label.vertex] = label_idx;
				if (--remaining == 0) {
					break;
				}
			}
			for (auto i = graph.out_offsets[label.vertex]; i < graph.out_offsets[label.vertex + 1]; i++) {
				auto &arc = graph.out_arcs[i];
				if (label.arc != INVALID_ARC && graph.out_arcs[label.arc].edge == arc.edge) {
					continue;
				}
				const auto state = automaton.Next(label.state, graph.edges[arc.edge].id);
				const auto cost = arc.cost + automaton.Penalty(state);
				const auto dist = label.dist + cost;
				if (dist == Infinity()) {
					continue;
				}
				const auto key = static_cast<uint64_t>(i) * state_count + state;
				auto found = label_index.find(key);
				if (found == label_index.end()) {
					const auto next_idx = UnsafeNumericCast<uint32_t>(labels.size());
					label_index[key] = next_idx;
					labels.push_back(Label {arc.head, state, dist, label_idx, i, cost});
					heap.Push(dist, next_idx);
				} else if (dist < labels[found->second].dist) {
					auto &existing = labels[found->second];
					existing.dist = dist;
					existing.pred_label = label_idx;
					existing.cost = cost;
					heap.Push(dist, found->second);
				}
			}
		}

		for (auto target : targets) {
			if (target == source || target_label[target] == INVALID_VERTEX) {
				continue;
			}
			Path path;
			path.start_vid = graph.vertex_ids[source];
			path.end_vid = graph.vertex_ids[target];
			path.stops.push_back(PathStop {path.end_vid, -1, 0, 0});
			for (auto idx = target_label[target]; labels[idx].pred_label != INVALID_VERTEX;) {
				auto &label = labels[idx];
				auto &arc = graph.out_arcs[label.arc];
				idx = label.pred_label;
				path.stops.push_back(
				    PathStop {graph.vertex_ids[labels[idx].vertex], graph.edges[arc.edge].id, label.cost, 0});
			}
			std::reverse(path.stops.begin(), path.stops.end());
			FinishPath(path);
			paths.push_back(std::move(path));
		}
	}

	void Compute(ClientContext &context, const RoutingInput &input, RoutingResult &result) const override {
		if (empty) {
			return;
		}
		Graph graph(ReadEdges(input, EDGE_COLUMN_INDEXES), directed);
		if (restrictions.empty()) {
			EmitPaths(result, ShortestPaths(context, graph, start_vids, end_vids));
			return;
		}
		RestrictionAutomaton automaton(restrictions);
		vector<int64_t> found;
		const auto sources = graph.LookupAll(start_vids, found);
		const auto targets = graph.LookupAll(end_vids, found);
		vector<uint8_t> target_flags(graph.VertexCount(), 0);
		for (auto target : targets) {
			target_flags[target] = 1;
		}
		vector<Path> paths;
		if (!targets.empty()) {
			for (auto source : sources) {
				Search(context, graph, automaton, source, targets, target_flags, paths);
			}
		}
		SortPaths(paths);
		EmitPaths(result, paths);
	}
};

unique_ptr<FunctionData> BindTurnRestricted(ClientContext &context, TableFunctionBindInput &input,
                                            vector<LogicalType> &return_types, vector<string> &names) {
	auto result = make_uniq<TurnRestrictedData>();
	RoutingBinder binder("pgr_trsp", input, 4);
	binder.BindColumns(*result, EDGE_COLUMN_SPECS, "edges");

	auto list = ParseStructList(binder, 1, "restrictions");
	if (!list.names.empty()) {
		idx_t path_idx = 0;
		idx_t cost_idx = 0;
		if (!list.Find("path", path_idx) || !list.Find("cost", cost_idx)) {
			throw BinderException("pgr_trsp: the restrictions need the fields \"path\" and \"cost\"");
		}
		auto &path_type = list.types[path_idx];
		if (path_type.id() != LogicalTypeId::LIST || !ListType::GetChildType(path_type).IsIntegral()) {
			throw BinderException("pgr_trsp: the field \"path\" of the restrictions must be a list of integers");
		}
		if (!list.types[cost_idx].IsNumeric()) {
			throw BinderException("pgr_trsp: the field \"cost\" of the restrictions must be numeric");
		}
		for (auto &row : list.rows) {
			if (row[path_idx].IsNull() || row[cost_idx].IsNull()) {
				throw BinderException("pgr_trsp: unexpected NULL value in the restrictions");
			}
			Restriction restriction;
			for (auto &edge : ListValue::GetChildren(row[path_idx])) {
				if (edge.IsNull()) {
					throw BinderException("pgr_trsp: unexpected NULL value in the restrictions");
				}
				restriction.path.push_back(edge.DefaultCastAs(LogicalType::BIGINT).GetValue<int64_t>());
			}
			restriction.cost = row[cost_idx].DefaultCastAs(LogicalType::DOUBLE).GetValue<double>();
			if (!(restriction.cost >= 0)) {
				throw BinderException("pgr_trsp: the cost of a restriction must be non-negative");
			}
			if (restriction.path.size() >= 2 && restriction.cost > 0) {
				result->restrictions.push_back(std::move(restriction));
			}
		}
	}
	result->start_vids = binder.VertexArgument(2, "start vids");
	result->end_vids = binder.VertexArgument(3, "end vids");
	result->directed = binder.Option("directed", LogicalType::BOOLEAN, Value::BOOLEAN(true)).GetValue<bool>();
	binder.Finish();
	result->empty = binder.has_null_argument;
	SetResultSchema(*result, return_types, names, PathSchema());
	return std::move(result);
}

} // namespace

void RegisterPointFunctions(ExtensionLoader &loader) {
	RegisterRoutingFunction(
	    loader, "pgr_withPoints", 3, BindWithPoints,
	    {{"driving_side", LogicalType::VARCHAR}, {"directed", LogicalType::BOOLEAN}, {"details", LogicalType::BOOLEAN}},
	    R"(
Shortest path(s) using Dijkstra's algorithm on a graph to which points located on the edges are added as temporary vertices.

`pgr_withPoints(edges, points, start vids, end vids, [driving_side], [directed := true, details := false])`

The edges are a table-valued argument with the columns `id`, `source`, `target`, `cost` and optionally `reverse_cost`, as for `pgr_dijkstra`.

The points are a constant list of structs with the fields:

| Field | Type | Description |
| --- | --- | --- |
| `pid` | integer | Optional. Identifier of the point, which becomes the vertex `-pid`. Defaults to the position in the list, starting from 1 |
| `edge_id` | integer | Identifier of the edge the point is on. Points on unknown edges are ignored |
| `fraction` | numeric | Position on the edge, between 0 (at `source`) and 1 (at `target`) |
| `side` | VARCHAR | Optional. `r`, `l` or `b` (default, also used for NULL): the side of the edge the point is on, looking from `source` to `target` |

A table-valued argument can only be used once per call, so the list has to be built beforehand, for example with `SET VARIABLE points = (SELECT list(p) FROM points_of_interest p)` and passed as `getvariable('points')`.

`start vids` and `end vids` are a single integer or a list of integers. Negative values designate points, positive values vertices of the graph.

`driving_side` is `r` (default on a directed graph), `l` or `b` (always used on an undirected graph). With right side driving a point on the right side of an edge can only be reached while travelling from `source` to `target`, and a point on the left side while travelling from `target` to `source`; left side driving is the opposite. It can be given as the fifth positional argument or by name.

When `details` is false, consecutive rows of a path that are on the same edge are merged, which hides the points that are passed along the way. When it is true every point that is passed is returned as a row with a negative `node`.

The result has the columns `seq`, `path_seq`, `start_vid`, `end_vid`, `node`, `edge`, `cost` and `agg_cost` of `pgr_dijkstra`, ordered by `start_vid` and `end_vid`.

Differences with pgRouting: the edges are a table-valued argument and the points a list of structs instead of two SQL strings, and the combinations signature is not available.
)",
	    R"(
SET VARIABLE points = (SELECT list(p) FROM (SELECT pid, edge_id, fraction, side FROM points_of_interest) p);

SELECT * FROM pgr_withPoints((SELECT id, source, target, cost, reverse_cost FROM edges), getvariable('points'), -1, [10, -3], 'r', details := true);
)");

	RegisterRoutingFunction(loader, "pgr_trsp", 3, BindTurnRestricted, {{"directed", LogicalType::BOOLEAN}},
	                        R"(
Shortest path(s) with turn restrictions.

`pgr_trsp(edges, restrictions, start vids, end vids, [directed := true])`

The edges are a table-valued argument with the columns `id`, `source`, `target`, `cost` and optionally `reverse_cost`, as for `pgr_dijkstra`.

The restrictions are a constant list of structs with the fields:

| Field | Type | Description |
| --- | --- | --- |
| `path` | list of integers | Sequence of edge identifiers that make up the restricted manoeuvre |
| `cost` | numeric | Cost that is added when the whole sequence is travelled. Use `'infinity'::DOUBLE` to forbid the manoeuvre |

Other fields, such as an identifier, are ignored. A table-valued argument can only be used once per call, so the list has to be built beforehand, for example with `SET VARIABLE restrictions = (SELECT list(r) FROM restrictions r)` and passed as `getvariable('restrictions')`.

`start vids` and `end vids` are a single integer or a list of integers.

The result has the columns `seq`, `path_seq`, `start_vid`, `end_vid`, `node`, `edge`, `cost` and `agg_cost` of `pgr_dijkstra`. The cost of a restriction is included in the `cost` of the edge that completes it. A vertex can appear more than once in a path when a detour is cheaper than a restricted manoeuvre, but as in pgRouting an edge is never followed by a U-turn on that same edge.

Differences with pgRouting: the edges are a table-valued argument and the restrictions a list of structs instead of two SQL strings; the combinations signature is not available; and the search tracks how much of each restriction has been travelled, so that the result is the cheapest path for restrictions of any length, including overlapping ones.
)",
	                        R"(
SET VARIABLE restrictions = (SELECT list(r) FROM (SELECT path, cost FROM restrictions) r);

SELECT * FROM pgr_trsp((SELECT id, source, target, cost, reverse_cost FROM edges), getvariable('restrictions'), 6, 10);
)");
}

} // namespace routing

} // namespace duckdb
