#include "spatial/modules/routing/routing_module.hpp"
#include "spatial/modules/routing/routing_search.hpp"

#include "duckdb/common/exception.hpp"

#include <algorithm>
#include <cmath>
#include <set>

namespace duckdb {

namespace routing {

const vector<ColumnSpec> EDGE_COLUMN_SPECS = {{"id", ColumnKind::INTEGER, true},
                                              {"source", ColumnKind::INTEGER, true},
                                              {"target", ColumnKind::INTEGER, true},
                                              {"cost", ColumnKind::NUMERIC, true},
                                              {"reverse_cost", ColumnKind::NUMERIC, false}};

const EdgeColumns EDGE_COLUMN_INDEXES = {0, 1, 2, 3, 4};

//----------------------------------------------------------------------------------------------------------------------
// Shared helpers
//----------------------------------------------------------------------------------------------------------------------
vector<std::pair<const char *, LogicalType>> PathSchema() {
	return {{"seq", LogicalType::INTEGER},      {"path_seq", LogicalType::INTEGER}, {"start_vid", LogicalType::BIGINT},
	        {"end_vid", LogicalType::BIGINT},   {"node", LogicalType::BIGINT},      {"edge", LogicalType::BIGINT},
	        {"cost", LogicalType::DOUBLE},      {"agg_cost", LogicalType::DOUBLE}};
}

void SortPaths(vector<Path> &paths) {
	std::sort(paths.begin(), paths.end(), [](const Path &a, const Path &b) {
		if (a.start_vid != b.start_vid) {
			return a.start_vid < b.start_vid;
		}
		return a.end_vid < b.end_vid;
	});
}

void EmitPaths(RoutingResult &result, const vector<Path> &paths) {
	int64_t seq = 0;
	for (auto &path : paths) {
		int64_t path_seq = 0;
		for (auto &stop : path.stops) {
			result.BeginRow();
			result.SetInteger(0, ++seq);
			result.SetInteger(1, ++path_seq);
			result.SetInteger(2, path.start_vid);
			result.SetInteger(3, path.end_vid);
			result.SetInteger(4, stop.node);
			result.SetInteger(5, stop.edge);
			result.SetDouble(6, stop.cost);
			result.SetDouble(7, stop.agg_cost);
		}
	}
}

static vector<uint8_t> MarkVertices(const Graph &graph, const vector<uint32_t> &vertices) {
	vector<uint8_t> flags(graph.VertexCount(), 0);
	for (auto vertex : vertices) {
		flags[vertex] = 1;
	}
	return flags;
}

vector<Path> ShortestPaths(ClientContext &context, Graph &graph, const vector<int64_t> &start_vids,
                           const vector<int64_t> &end_vids) {
	vector<Path> paths;
	vector<int64_t> found;
	const auto sources = graph.LookupAll(start_vids, found);
	const auto targets = graph.LookupAll(end_vids, found);
	if (sources.empty() || targets.empty()) {
		return paths;
	}
	SearchSpace space(graph.VertexCount());
	SearchOptions options;
	Path path;
	if (targets.size() < sources.size()) {
		graph.BuildReverse();
		const auto flags = MarkVertices(graph, sources);
		options.targets = &flags;
		options.target_count = sources.size();
		for (auto target : targets) {
			CheckInterrupt(context);
			RunDijkstra(graph.in_offsets, graph.in_arcs, target, space, options);
			for (auto source : sources) {
				if (BackwardPath(graph, space, source, target, path)) {
					paths.push_back(path);
				}
			}
		}
	} else {
		const auto flags = MarkVertices(graph, targets);
		options.targets = &flags;
		options.target_count = targets.size();
		for (auto source : sources) {
			CheckInterrupt(context);
			RunDijkstra(graph.out_offsets, graph.out_arcs, source, space, options);
			for (auto target : targets) {
				if (ForwardPath(graph, space, source, target, path)) {
					paths.push_back(path);
				}
			}
		}
	}
	SortPaths(paths);
	return paths;
}

struct CostRow {
	int64_t start_vid;
	int64_t end_vid;
	double agg_cost;
};

static vector<CostRow> ShortestCosts(ClientContext &context, Graph &graph, const vector<int64_t> &start_vids,
                                     const vector<int64_t> &end_vids) {
	vector<CostRow> rows;
	vector<int64_t> found;
	const auto sources = graph.LookupAll(start_vids, found);
	const auto targets = graph.LookupAll(end_vids, found);
	if (sources.empty() || targets.empty()) {
		return rows;
	}
	SearchSpace space(graph.VertexCount());
	SearchOptions options;
	if (targets.size() < sources.size()) {
		graph.BuildReverse();
		const auto flags = MarkVertices(graph, sources);
		options.targets = &flags;
		options.target_count = sources.size();
		for (auto target : targets) {
			CheckInterrupt(context);
			RunDijkstra(graph.in_offsets, graph.in_arcs, target, space, options);
			for (auto source : sources) {
				if (source == target || !space.Reached(source)) {
					continue;
				}
				// Sum in travel order so that the value is identical to the agg_cost of the matching path
				double cost = 0;
				for (auto vertex = source; vertex != target; vertex = space.pred_vertex[vertex]) {
					cost += graph.in_arcs[space.pred_arc[vertex]].cost;
				}
				rows.push_back(CostRow {graph.vertex_ids[source], graph.vertex_ids[target], cost});
			}
		}
	} else {
		const auto flags = MarkVertices(graph, targets);
		options.targets = &flags;
		options.target_count = targets.size();
		for (auto source : sources) {
			CheckInterrupt(context);
			RunDijkstra(graph.out_offsets, graph.out_arcs, source, space, options);
			for (auto target : targets) {
				if (source == target || !space.Reached(target)) {
					continue;
				}
				rows.push_back(CostRow {graph.vertex_ids[source], graph.vertex_ids[target], space.dist[target]});
			}
		}
	}
	std::sort(rows.begin(), rows.end(), [](const CostRow &a, const CostRow &b) {
		if (a.start_vid != b.start_vid) {
			return a.start_vid < b.start_vid;
		}
		return a.end_vid < b.end_vid;
	});
	return rows;
}

bool StructListArgument::Find(const char *name, idx_t &index) const {
	for (idx_t i = 0; i < names.size(); i++) {
		if (StringUtil::CIEquals(names[i], name)) {
			index = i;
			return true;
		}
	}
	return false;
}

StructListArgument ParseStructList(RoutingBinder &binder, idx_t argument, const char *argument_name) {
	StructListArgument result;
	auto &value = binder.Argument(argument);
	auto &type = value.type();
	if (type.id() != LogicalTypeId::LIST) {
		if (value.IsNull()) {
			return result;
		}
		throw BinderException("%s: argument \"%s\" must be a list of structs, got %s", binder.name, argument_name,
		                      type.ToString());
	}
	auto &child_type = ListType::GetChildType(type);
	if (child_type.id() != LogicalTypeId::STRUCT) {
		if (value.IsNull() || ListValue::GetChildren(value).empty()) {
			return result;
		}
		throw BinderException("%s: argument \"%s\" must be a list of structs, got %s", binder.name, argument_name,
		                      type.ToString());
	}
	for (auto &child : StructType::GetChildTypes(child_type)) {
		result.names.push_back(child.first);
		result.types.push_back(child.second);
	}
	if (value.IsNull()) {
		return result;
	}
	for (auto &row : ListValue::GetChildren(value)) {
		if (row.IsNull()) {
			continue;
		}
		result.rows.push_back(StructValue::GetChildren(row));
	}
	return result;
}

namespace {

//----------------------------------------------------------------------------------------------------------------------
// Goal directed and bidirectional searches
//----------------------------------------------------------------------------------------------------------------------
struct Heuristic {
	vector<double> x;
	vector<double> y;
	int32_t type = 0;
	double scale = 1;

	double operator()(uint32_t from, uint32_t to) const {
		const auto dx = x[to] - x[from];
		const auto dy = y[to] - y[from];
		switch (type) {
		case 1:
			return std::fabs(MaxValue(dx, dy)) * scale;
		case 2:
			return std::fabs(MinValue(dx, dy)) * scale;
		case 3:
			return (dx * dx + dy * dy) * scale * scale;
		case 4:
			return std::sqrt(dx * dx + dy * dy) * scale;
		case 5:
			return (std::fabs(dx) + std::fabs(dy)) * scale;
		default:
			return 0;
		}
	}
};

bool RunAStar(const Graph &graph, const Heuristic &heuristic, double epsilon, uint32_t source, uint32_t target,
              SearchSpace &space) {
	space.Reset();
	space.Relax(source, 0, INVALID_VERTEX, INVALID_ARC);
	space.heap.Push(epsilon * heuristic(source, target), source);
	while (!space.heap.Empty()) {
		const auto entry = space.heap.Pop();
		const auto vertex = entry.vertex;
		if (entry.key > space.dist[vertex] + epsilon * heuristic(vertex, target)) {
			continue;
		}
		if (vertex == target) {
			return true;
		}
		for (auto i = graph.out_offsets[vertex]; i < graph.out_offsets[vertex + 1]; i++) {
			auto &arc = graph.out_arcs[i];
			const auto distance = space.dist[vertex] + arc.cost;
			if (space.Relax(arc.head, distance, vertex, i)) {
				space.heap.Push(distance + epsilon * heuristic(arc.head, target), arc.head);
			}
		}
	}
	return false;
}

//! Bidirectional search with the average potential p(v) = (h(v, target) - h(source, v)) / 2, which keeps the forward
//! and backward reduced costs identical. Without a heuristic this is a plain bidirectional Dijkstra.
class BidirectionalSearch {
public:
	BidirectionalSearch(const Graph &graph_p, const Heuristic *heuristic_p, double epsilon_p)
	    : graph(graph_p), heuristic(heuristic_p), epsilon(epsilon_p), forward(graph_p.VertexCount()),
	      backward(graph_p.VertexCount()) {
	}

	bool Run(uint32_t source_p, uint32_t target_p, Path &path) {
		source = source_p;
		target = target_p;
		forward.Reset();
		backward.Reset();
		best = Infinity();
		meeting = INVALID_VERTEX;

		forward.Relax(source, 0, INVALID_VERTEX, INVALID_ARC);
		forward.heap.Push(Potential(source), source);
		backward.Relax(target, 0, INVALID_VERTEX, INVALID_ARC);
		backward.heap.Push(-Potential(target), target);

		while (true) {
			DropStale(forward, 1);
			DropStale(backward, -1);
			if (forward.heap.Empty() || backward.heap.Empty()) {
				break;
			}
			const auto forward_key = forward.heap.Top().key;
			const auto backward_key = backward.heap.Top().key;
			if (forward_key + backward_key >= best) {
				break;
			}
			if (forward_key <= backward_key) {
				Expand(forward, backward, graph.out_offsets, graph.out_arcs, 1);
			} else {
				Expand(backward, forward, graph.in_offsets, graph.in_arcs, -1);
			}
		}
		if (meeting == INVALID_VERTEX) {
			return false;
		}
		BuildPath(path);
		return true;
	}

private:
	double Potential(uint32_t vertex) const {
		if (!heuristic) {
			return 0;
		}
		return 0.5 * epsilon * ((*heuristic)(vertex, target) - (*heuristic)(source, vertex));
	}

	void DropStale(SearchSpace &space, double sign) {
		while (!space.heap.Empty()) {
			auto &top = space.heap.Top();
			if (top.key > space.dist[top.vertex] + sign * Potential(top.vertex)) {
				space.heap.Pop();
			} else {
				break;
			}
		}
	}

	void Expand(SearchSpace &space, SearchSpace &other, const vector<uint32_t> &offsets, const vector<Arc> &arcs,
	            double sign) {
		const auto vertex = space.heap.Pop().vertex;
		for (auto i = offsets[vertex]; i < offsets[vertex + 1]; i++) {
			auto &arc = arcs[i];
			const auto distance = space.dist[vertex] + arc.cost;
			if (space.Relax(arc.head, distance, vertex, i)) {
				space.heap.Push(distance + sign * Potential(arc.head), arc.head);
			}
			if (other.Reached(arc.head)) {
				const auto total = space.dist[arc.head] + other.dist[arc.head];
				if (total < best) {
					best = total;
					meeting = arc.head;
				}
			}
		}
	}

	void BuildPath(Path &path) {
		path.start_vid = graph.vertex_ids[source];
		path.end_vid = graph.vertex_ids[target];
		path.stops.clear();
		for (auto vertex = meeting; vertex != source;) {
			auto &arc = graph.out_arcs[forward.pred_arc[vertex]];
			vertex = forward.pred_vertex[vertex];
			path.stops.push_back(PathStop {graph.vertex_ids[vertex], graph.edges[arc.edge].id, arc.cost, 0});
		}
		std::reverse(path.stops.begin(), path.stops.end());
		for (auto vertex = meeting; vertex != target;) {
			auto &arc = graph.in_arcs[backward.pred_arc[vertex]];
			path.stops.push_back(PathStop {graph.vertex_ids[vertex], graph.edges[arc.edge].id, arc.cost, 0});
			vertex = backward.pred_vertex[vertex];
		}
		path.stops.push_back(PathStop {graph.vertex_ids[target], -1, 0, 0});
		FinishPath(path);
	}

	const Graph &graph;
	const Heuristic *heuristic;
	double epsilon;
	SearchSpace forward;
	SearchSpace backward;
	uint32_t source = 0;
	uint32_t target = 0;
	double best = 0;
	uint32_t meeting = INVALID_VERTEX;
};

//----------------------------------------------------------------------------------------------------------------------
// pgr_dijkstra, pgr_aStar, pgr_bdDijkstra, pgr_bdAstar, pgr_dijkstraCost, pgr_dijkstraCostMatrix
//----------------------------------------------------------------------------------------------------------------------
enum class PathAlgorithm : uint8_t { DIJKSTRA, ASTAR, BD_DIJKSTRA, BD_ASTAR };
enum class PathOutput : uint8_t { PATHS, COSTS, COST_MATRIX };

struct ShortestPathData : public RoutingBindData {
	PathAlgorithm algorithm = PathAlgorithm::DIJKSTRA;
	PathOutput output = PathOutput::PATHS;
	vector<int64_t> start_vids;
	vector<int64_t> end_vids;
	bool directed = true;
	int32_t heuristic = 5;
	double factor = 1;
	double epsilon = 1;
	bool empty = false;

	bool UsesHeuristic() const {
		return algorithm == PathAlgorithm::ASTAR || algorithm == PathAlgorithm::BD_ASTAR;
	}

	void Compute(ClientContext &context, const RoutingInput &input, RoutingResult &result) const override {
		if (empty) {
			return;
		}
		Graph graph(ReadEdges(input, EDGE_COLUMN_INDEXES), directed);
		if (output != PathOutput::PATHS) {
			for (auto &row : ShortestCosts(context, graph, start_vids, end_vids)) {
				result.BeginRow();
				result.SetInteger(0, row.start_vid);
				result.SetInteger(1, row.end_vid);
				result.SetDouble(2, row.agg_cost);
			}
			return;
		}
		if (algorithm == PathAlgorithm::DIJKSTRA) {
			EmitPaths(result, ShortestPaths(context, graph, start_vids, end_vids));
			return;
		}

		Heuristic h;
		if (UsesHeuristic()) {
			ReadCoordinates(input, graph, h);
		}
		vector<int64_t> found;
		const auto sources = graph.LookupAll(start_vids, found);
		const auto targets = graph.LookupAll(end_vids, found);
		vector<Path> paths;
		Path path;
		if (algorithm == PathAlgorithm::ASTAR) {
			SearchSpace space(graph.VertexCount());
			for (auto source : sources) {
				for (auto target : targets) {
					CheckInterrupt(context);
					if (source != target && RunAStar(graph, h, epsilon, source, target, space) &&
					    ForwardPath(graph, space, source, target, path)) {
						paths.push_back(path);
					}
				}
			}
		} else {
			graph.BuildReverse();
			BidirectionalSearch search(graph, algorithm == PathAlgorithm::BD_ASTAR ? &h : nullptr, epsilon);
			for (auto source : sources) {
				for (auto target : targets) {
					CheckInterrupt(context);
					if (source != target && search.Run(source, target, path)) {
						paths.push_back(path);
					}
				}
			}
		}
		SortPaths(paths);
		EmitPaths(result, paths);
	}

	void ReadCoordinates(const RoutingInput &input, const Graph &graph, Heuristic &h) const {
		h.type = heuristic;
		h.scale = factor;
		const auto nan = std::numeric_limits<double>::quiet_NaN();
		h.x.assign(graph.VertexCount(), nan);
		h.y.assign(graph.VertexCount(), nan);
		auto &sources = input.Get(1).integers;
		auto &targets = input.Get(2).integers;
		auto &x1 = input.Get(5).numerics;
		auto &y1 = input.Get(6).numerics;
		auto &x2 = input.Get(7).numerics;
		auto &y2 = input.Get(8).numerics;
		for (idx_t i = 0; i < input.row_count; i++) {
			SetCoordinate(graph, h, sources[i], x1[i], y1[i]);
			SetCoordinate(graph, h, targets[i], x2[i], y2[i]);
		}
	}

	// A vertex that is given different coordinates by different edges keeps the smallest one, whatever the row order
	static void SetCoordinate(const Graph &graph, Heuristic &h, int64_t vid, double x, double y) {
		uint32_t vertex;
		if (!graph.Lookup(vid, vertex)) {
			return;
		}
		if (std::isnan(h.x[vertex]) || x < h.x[vertex] || (x == h.x[vertex] && y < h.y[vertex])) {
			h.x[vertex] = x;
			h.y[vertex] = y;
		}
	}
};

template <PathAlgorithm ALGORITHM, PathOutput OUTPUT>
unique_ptr<FunctionData> BindShortestPath(ClientContext &context, TableFunctionBindInput &input,
                                          vector<LogicalType> &return_types, vector<string> &names) {
	auto result = make_uniq<ShortestPathData>();
	result->algorithm = ALGORITHM;
	result->output = OUTPUT;
	const auto name = input.table_function.name.c_str();

	RoutingBinder binder(name, input, OUTPUT == PathOutput::COST_MATRIX ? 2 : 3);
	auto specs = EDGE_COLUMN_SPECS;
	if (result->UsesHeuristic()) {
		specs.push_back({"x1", ColumnKind::NUMERIC, true});
		specs.push_back({"y1", ColumnKind::NUMERIC, true});
		specs.push_back({"x2", ColumnKind::NUMERIC, true});
		specs.push_back({"y2", ColumnKind::NUMERIC, true});
	}
	binder.BindColumns(*result, specs, "edges");

	if (OUTPUT == PathOutput::COST_MATRIX) {
		result->start_vids = binder.VertexArgument(1, "vids");
		result->end_vids = result->start_vids;
	} else {
		result->start_vids = binder.VertexArgument(1, "start vids");
		result->end_vids = binder.VertexArgument(2, "end vids");
	}
	result->directed = binder.Option("directed", LogicalType::BOOLEAN, Value::BOOLEAN(true)).GetValue<bool>();
	if (result->UsesHeuristic()) {
		result->heuristic = binder.Option("heuristic", LogicalType::INTEGER, Value::INTEGER(5)).GetValue<int32_t>();
		result->factor = binder.Option("factor", LogicalType::DOUBLE, Value::DOUBLE(1)).GetValue<double>();
		result->epsilon = binder.Option("epsilon", LogicalType::DOUBLE, Value::DOUBLE(1)).GetValue<double>();
		if (result->heuristic < 0 || result->heuristic > 5) {
			throw BinderException("%s: unknown heuristic %d, valid values are 0 to 5", name, result->heuristic);
		}
		if (!(result->factor > 0)) {
			throw BinderException("%s: factor must be positive", name);
		}
		if (!(result->epsilon >= 1)) {
			throw BinderException("%s: epsilon must be greater than or equal to 1", name);
		}
	}
	binder.Finish();
	result->empty = binder.has_null_argument;

	if (OUTPUT == PathOutput::PATHS) {
		SetResultSchema(*result, return_types, names, PathSchema());
	} else {
		SetResultSchema(
		    *result, return_types, names,
		    {{"start_vid", LogicalType::BIGINT}, {"end_vid", LogicalType::BIGINT}, {"agg_cost", LogicalType::DOUBLE}});
	}
	return std::move(result);
}

//----------------------------------------------------------------------------------------------------------------------
// pgr_drivingDistance
//----------------------------------------------------------------------------------------------------------------------
struct DrivingDistanceData : public RoutingBindData {
	vector<int64_t> start_vids;
	double distance = 0;
	bool directed = true;
	bool equicost = false;
	bool empty = false;

	struct Row {
		uint32_t start;
		uint32_t depth;
		uint32_t node;
		uint32_t pred;
		int64_t edge;
		double cost;
		double agg_cost;
	};

	void Compute(ClientContext &context, const RoutingInput &input, RoutingResult &result) const override {
		if (empty) {
			return;
		}
		Graph graph(ReadEdges(input, EDGE_COLUMN_INDEXES), directed);
		vector<int64_t> found;
		const auto sources = graph.LookupAll(start_vids, found);

		SearchSpace space(graph.VertexCount());
		SearchOptions options;
		options.limit = distance;
		vector<uint32_t> depth(graph.VertexCount(), INVALID_VERTEX);
		vector<uint32_t> chain;
		vector<Row> rows;

		// equicost: nearest start per vertex, the smallest start vid wins ties
		vector<double> best_cost;
		vector<uint32_t> best_start;
		if (equicost) {
			best_cost.assign(graph.VertexCount(), Infinity());
			best_start.assign(graph.VertexCount(), INVALID_VERTEX);
		}

		for (auto source : sources) {
			CheckInterrupt(context);
			RunDijkstra(graph.out_offsets, graph.out_arcs, source, space, options);
			depth[source] = 0;
			for (auto vertex : space.touched) {
				chain.clear();
				auto current = vertex;
				while (depth[current] == INVALID_VERTEX) {
					chain.push_back(current);
					current = space.pred_vertex[current];
				}
				auto level = depth[current];
				while (!chain.empty()) {
					depth[chain.back()] = ++level;
					chain.pop_back();
				}
			}
			const auto first_row = rows.size();
			for (auto vertex : space.touched) {
				Row row;
				row.start = source;
				row.depth = depth[vertex];
				row.node = vertex;
				row.agg_cost = space.dist[vertex];
				if (vertex == source) {
					row.pred = source;
					row.edge = -1;
					row.cost = 0;
				} else {
					auto &arc = graph.out_arcs[space.pred_arc[vertex]];
					row.pred = space.pred_vertex[vertex];
					row.edge = graph.edges[arc.edge].id;
					row.cost = arc.cost;
				}
				rows.push_back(row);
				if (equicost && row.agg_cost < best_cost[vertex]) {
					best_cost[vertex] = row.agg_cost;
					best_start[vertex] = source;
				}
			}
			for (auto vertex : space.touched) {
				depth[vertex] = INVALID_VERTEX;
			}
			std::sort(rows.begin() + NumericCast<int64_t>(first_row), rows.end(), [](const Row &a, const Row &b) {
				if (a.depth != b.depth) {
					return a.depth < b.depth;
				}
				return a.node < b.node;
			});
		}

		int64_t seq = 0;
		for (auto &row : rows) {
			if (equicost && best_start[row.node] != row.start) {
				continue;
			}
			result.BeginRow();
			result.SetInteger(0, ++seq);
			result.SetInteger(1, row.depth);
			result.SetInteger(2, graph.vertex_ids[row.start]);
			result.SetInteger(3, graph.vertex_ids[row.pred]);
			result.SetInteger(4, graph.vertex_ids[row.node]);
			result.SetInteger(5, row.edge);
			result.SetDouble(6, row.cost);
			result.SetDouble(7, row.agg_cost);
		}
	}
};

unique_ptr<FunctionData> BindDrivingDistance(ClientContext &context, TableFunctionBindInput &input,
                                             vector<LogicalType> &return_types, vector<string> &names) {
	auto result = make_uniq<DrivingDistanceData>();
	RoutingBinder binder("pgr_drivingDistance", input, 3);
	binder.BindColumns(*result, EDGE_COLUMN_SPECS, "edges");
	result->start_vids = binder.VertexArgument(1, "root vids");
	result->distance = binder.DoubleArgument(2, "distance");
	result->directed = binder.Option("directed", LogicalType::BOOLEAN, Value::BOOLEAN(true)).GetValue<bool>();
	result->equicost = binder.Option("equicost", LogicalType::BOOLEAN, Value::BOOLEAN(false)).GetValue<bool>();
	binder.Finish();
	result->empty = binder.has_null_argument;
	if (!result->empty && !(result->distance >= 0)) {
		throw BinderException("pgr_drivingDistance: distance must be non-negative");
	}
	SetResultSchema(*result, return_types, names,
	                {{"seq", LogicalType::BIGINT},
	                 {"depth", LogicalType::BIGINT},
	                 {"start_vid", LogicalType::BIGINT},
	                 {"pred", LogicalType::BIGINT},
	                 {"node", LogicalType::BIGINT},
	                 {"edge", LogicalType::BIGINT},
	                 {"cost", LogicalType::DOUBLE},
	                 {"agg_cost", LogicalType::DOUBLE}});
	return std::move(result);
}

//----------------------------------------------------------------------------------------------------------------------
// pgr_KSP
//----------------------------------------------------------------------------------------------------------------------
struct CandidatePath {
	double cost = 0;
	vector<uint32_t> vertices;
	vector<uint32_t> arcs;

	bool operator<(const CandidatePath &other) const {
		if (cost != other.cost) {
			return cost < other.cost;
		}
		if (vertices != other.vertices) {
			return vertices < other.vertices;
		}
		return arcs < other.arcs;
	}
};

struct KShortestPathData : public RoutingBindData {
	vector<int64_t> start_vids;
	vector<int64_t> end_vids;
	int64_t k = 0;
	bool directed = true;
	bool heap_paths = false;
	bool empty = false;

	static bool Extract(const Graph &graph, const SearchSpace &space, uint32_t source, uint32_t target,
	                    CandidatePath &path) {
		if (!space.Reached(target)) {
			return false;
		}
		for (auto vertex = target; vertex != source; vertex = space.pred_vertex[vertex]) {
			path.vertices.push_back(vertex);
			path.arcs.push_back(space.pred_arc[vertex]);
		}
		path.vertices.push_back(source);
		std::reverse(path.vertices.begin(), path.vertices.end());
		std::reverse(path.arcs.begin(), path.arcs.end());
		return true;
	}

	static double PathCost(const Graph &graph, const CandidatePath &path) {
		double cost = 0;
		for (auto arc : path.arcs) {
			cost += graph.out_arcs[arc].cost;
		}
		return cost;
	}

	void Yen(ClientContext &context, const Graph &graph, uint32_t source, uint32_t target, SearchSpace &space,
	         vector<uint8_t> &blocked_vertices, vector<uint8_t> &blocked_arcs, vector<CandidatePath> &accepted) const {
		accepted.clear();
		if (k == 0 || source == target) {
			return;
		}
		vector<uint8_t> flags(graph.VertexCount(), 0);
		flags[target] = 1;
		SearchOptions options;
		options.targets = &flags;
		options.target_count = 1;

		CandidatePath first;
		RunDijkstra(graph.out_offsets, graph.out_arcs, source, space, options);
		if (!Extract(graph, space, source, target, first)) {
			return;
		}
		first.cost = PathCost(graph, first);
		accepted.push_back(std::move(first));

		options.blocked_vertices = &blocked_vertices;
		options.blocked_arcs = &blocked_arcs;
		std::set<CandidatePath> candidates;
		vector<uint32_t> blocked_arc_list;

		while (accepted.size() < NumericCast<idx_t>(k)) {
			CheckInterrupt(context);
			const auto previous = accepted.back();
			for (idx_t i = 0; i + 1 < previous.vertices.size(); i++) {
				const auto spur = previous.vertices[i];
				blocked_arc_list.clear();
				for (auto &path : accepted) {
					if (path.arcs.size() > i && std::equal(previous.arcs.begin(), previous.arcs.begin() + i,
					                                       path.arcs.begin())) {
						blocked_arc_list.push_back(path.arcs[i]);
					}
				}
				for (auto arc : blocked_arc_list) {
					blocked_arcs[arc] = 1;
				}
				for (idx_t j = 0; j < i; j++) {
					blocked_vertices[previous.vertices[j]] = 1;
				}

				RunDijkstra(graph.out_offsets, graph.out_arcs, spur, space, options);
				CandidatePath candidate;
				if (Extract(graph, space, spur, target, candidate)) {
					candidate.vertices.insert(candidate.vertices.begin(), previous.vertices.begin(),
					                          previous.vertices.begin() + i);
					candidate.arcs.insert(candidate.arcs.begin(), previous.arcs.begin(), previous.arcs.begin() + i);
					candidate.cost = PathCost(graph, candidate);
					candidates.insert(std::move(candidate));
				}

				for (auto arc : blocked_arc_list) {
					blocked_arcs[arc] = 0;
				}
				for (idx_t j = 0; j < i; j++) {
					blocked_vertices[previous.vertices[j]] = 0;
				}
			}
			while (!candidates.empty() &&
			       std::find_if(accepted.begin(), accepted.end(), [&](const CandidatePath &path) {
				       return path.arcs == candidates.begin()->arcs;
			       }) != accepted.end()) {
				candidates.erase(candidates.begin());
			}
			if (candidates.empty()) {
				break;
			}
			accepted.push_back(*candidates.begin());
			candidates.erase(candidates.begin());
		}
		if (heap_paths) {
			for (auto &candidate : candidates) {
				if (std::find_if(accepted.begin(), accepted.end(), [&](const CandidatePath &path) {
					    return path.arcs == candidate.arcs;
				    }) == accepted.end()) {
					accepted.push_back(candidate);
				}
			}
		}
	}

	void Compute(ClientContext &context, const RoutingInput &input, RoutingResult &result) const override {
		if (empty) {
			return;
		}
		Graph graph(ReadEdges(input, EDGE_COLUMN_INDEXES), directed);
		vector<int64_t> found;
		const auto sources = graph.LookupAll(start_vids, found);
		const auto targets = graph.LookupAll(end_vids, found);

		SearchSpace space(graph.VertexCount());
		vector<uint8_t> blocked_vertices(graph.VertexCount(), 0);
		vector<uint8_t> blocked_arcs(graph.out_arcs.size(), 0);
		vector<CandidatePath> accepted;
		int64_t seq = 0;
		int64_t path_id = 0;
		for (auto source : sources) {
			for (auto target : targets) {
				Yen(context, graph, source, target, space, blocked_vertices, blocked_arcs, accepted);
				for (auto &path : accepted) {
					path_id++;
					double agg_cost = 0;
					for (idx_t i = 0; i < path.vertices.size(); i++) {
						const auto last = i + 1 == path.vertices.size();
						const auto cost = last ? 0 : graph.out_arcs[path.arcs[i]].cost;
						result.BeginRow();
						result.SetInteger(0, ++seq);
						result.SetInteger(1, path_id);
						result.SetInteger(2, NumericCast<int64_t>(i + 1));
						result.SetInteger(3, graph.vertex_ids[source]);
						result.SetInteger(4, graph.vertex_ids[target]);
						result.SetInteger(5, graph.vertex_ids[path.vertices[i]]);
						result.SetInteger(6, last ? -1 : graph.edges[graph.out_arcs[path.arcs[i]].edge].id);
						result.SetDouble(7, cost);
						result.SetDouble(8, agg_cost);
						agg_cost += cost;
					}
				}
			}
		}
	}
};

unique_ptr<FunctionData> BindKShortestPath(ClientContext &context, TableFunctionBindInput &input,
                                           vector<LogicalType> &return_types, vector<string> &names) {
	auto result = make_uniq<KShortestPathData>();
	RoutingBinder binder("pgr_KSP", input, 4);
	binder.BindColumns(*result, EDGE_COLUMN_SPECS, "edges");
	result->start_vids = binder.VertexArgument(1, "start vids");
	result->end_vids = binder.VertexArgument(2, "end vids");
	result->k = binder.IntegerArgument(3, "K");
	result->directed = binder.Option("directed", LogicalType::BOOLEAN, Value::BOOLEAN(true)).GetValue<bool>();
	result->heap_paths = binder.Option("heap_paths", LogicalType::BOOLEAN, Value::BOOLEAN(false)).GetValue<bool>();
	binder.Finish();
	result->empty = binder.has_null_argument;
	if (result->k < 0) {
		throw BinderException("pgr_KSP: K must be non-negative");
	}
	SetResultSchema(*result, return_types, names,
	                {{"seq", LogicalType::INTEGER},
	                 {"path_id", LogicalType::INTEGER},
	                 {"path_seq", LogicalType::INTEGER},
	                 {"start_vid", LogicalType::BIGINT},
	                 {"end_vid", LogicalType::BIGINT},
	                 {"node", LogicalType::BIGINT},
	                 {"edge", LogicalType::BIGINT},
	                 {"cost", LogicalType::DOUBLE},
	                 {"agg_cost", LogicalType::DOUBLE}});
	return std::move(result);
}

//----------------------------------------------------------------------------------------------------------------------
// Documentation
//----------------------------------------------------------------------------------------------------------------------
const char *const EDGES_DOC = R"(
The edges are passed as a table-valued argument, i.e. a parenthesised subquery, and its columns are matched by name:

| Column | Type | Description |
| --- | --- | --- |
| `id` | integer | Identifier of the edge |
| `source` | integer | Identifier of the first end point vertex |
| `target` | integer | Identifier of the second end point vertex |
| `cost` | numeric | Weight of the edge (`source`, `target`). A negative value means the edge does not exist in that direction |
| `reverse_cost` | numeric | Optional. Weight of the edge (`target`, `source`). A negative value, or a missing column, means the edge does not exist in that direction |

When `directed` is false every non-negative `cost` and `reverse_cost` is usable in both directions.
A NULL in any of these columns raises an error.)";

const char *const VIDS_DOC = R"(
`start vids` and `end vids` are either a single integer or a list of integers, which covers the one-to-one, one-to-many, many-to-one and many-to-many signatures of pgRouting. Duplicates are ignored and vertices that are not part of the graph are skipped.
They have to be constants: a literal, a prepared statement parameter or `getvariable('name')`.)";

const char *const PATH_RESULT_DOC = R"(
The result has one row per vertex of each path, ordered by `start_vid`, `end_vid` and position in the path:

| Column | Type | Description |
| --- | --- | --- |
| `seq` | INTEGER | Sequential value starting from 1 |
| `path_seq` | INTEGER | Position in the path, starting from 1 |
| `start_vid` | BIGINT | Identifier of the starting vertex of the path |
| `end_vid` | BIGINT | Identifier of the ending vertex of the path |
| `node` | BIGINT | Identifier of the vertex at this position |
| `edge` | BIGINT | Identifier of the edge used to go to the next vertex, -1 for the last vertex |
| `cost` | DOUBLE | Cost to traverse `edge`, 0 for the last vertex |
| `agg_cost` | DOUBLE | Aggregate cost from `start_vid` to `node` |

No rows are returned for a pair whose end vertex cannot be reached, or whose start and end vertex are the same.)";

const char *const DEVIATION_DOC = R"(
Differences with pgRouting: the edges are a table-valued argument instead of an SQL string, so the query runs inside the calling transaction and can read CTEs and temporary tables; the combinations signature is not available; and when several paths have the same cost the one that is returned may differ from pgRouting's choice (it is deterministic and does not depend on the order of the input rows).)";

string Doc(const string &head, const vector<const char *> &parts) {
	string result = head;
	for (auto part : parts) {
		result += "\n";
		result += part;
	}
	return result;
}

} // namespace

//----------------------------------------------------------------------------------------------------------------------
// Register
//----------------------------------------------------------------------------------------------------------------------
void RegisterPathFunctions(ExtensionLoader &loader) {
	const vector<NamedOption> directed_option = {{"directed", LogicalType::BOOLEAN}};
	const vector<NamedOption> astar_options = {{"directed", LogicalType::BOOLEAN},
	                                           {"heuristic", LogicalType::INTEGER},
	                                           {"factor", LogicalType::DOUBLE},
	                                           {"epsilon", LogicalType::DOUBLE}};

	const char *const astar_doc = R"(
The edges additionally need the numeric columns `x1`, `y1` (coordinates of the `source` vertex) and `x2`, `y2` (coordinates of the `target` vertex).

Options:

- `heuristic` (INTEGER, default 5): 0: `h(v) = 0`, 1: `abs(max(dx, dy))`, 2: `abs(min(dx, dy))`, 3: `dx * dx + dy * dy`, 4: `sqrt(dx * dx + dy * dy)`, 5: `abs(dx) + abs(dy)`
- `factor` (DOUBLE, default 1): multiplier that brings the heuristic to the unit of the costs, must be positive
- `epsilon` (DOUBLE, default 1): weight of the heuristic, must be greater than or equal to 1. A larger value is faster and less accurate

The path is a shortest path only when the scaled heuristic never overestimates the remaining cost. With many start or end vertices one search is run per pair.)";

	RegisterRoutingFunction(
	    loader, "pgr_dijkstra", 2, BindShortestPath<PathAlgorithm::DIJKSTRA, PathOutput::PATHS>, directed_option,
	    Doc(R"(
Shortest path(s) using Dijkstra's algorithm.

`pgr_dijkstra(edges, start vids, end vids, [directed := true])`)",
	        {EDGES_DOC, VIDS_DOC, PATH_RESULT_DOC, DEVIATION_DOC})
	        .c_str(),
	    R"(
SELECT * FROM pgr_dijkstra((SELECT id, source, target, cost, reverse_cost FROM edges), 6, 10);

-- one to many on an undirected graph
SELECT * FROM pgr_dijkstra((SELECT id, source, target, cost, reverse_cost FROM edges), 6, [10, 17], directed := false);
)");

	RegisterRoutingFunction(
	    loader, "pgr_aStar", 2, BindShortestPath<PathAlgorithm::ASTAR, PathOutput::PATHS>, astar_options,
	    Doc(R"(
Shortest path(s) using the A* algorithm.

`pgr_aStar(edges, start vids, end vids, [directed := true, heuristic := 5, factor := 1, epsilon := 1])`)",
	        {EDGES_DOC, astar_doc, VIDS_DOC, PATH_RESULT_DOC, DEVIATION_DOC})
	        .c_str(),
	    R"(
SELECT * FROM pgr_aStar((SELECT id, source, target, cost, reverse_cost, x1, y1, x2, y2 FROM edges), 6, 10, heuristic := 4);
)");

	RegisterRoutingFunction(
	    loader, "pgr_bdDijkstra", 2, BindShortestPath<PathAlgorithm::BD_DIJKSTRA, PathOutput::PATHS>, directed_option,
	    Doc(R"(
Shortest path(s) using a bidirectional Dijkstra search, which grows one search from the start vertex and one from the end vertex. One search is run per pair of start and end vertices.

`pgr_bdDijkstra(edges, start vids, end vids, [directed := true])`)",
	        {EDGES_DOC, VIDS_DOC, PATH_RESULT_DOC, DEVIATION_DOC})
	        .c_str(),
	    R"(
SELECT * FROM pgr_bdDijkstra((SELECT id, source, target, cost, reverse_cost FROM edges), 6, 10);
)");

	RegisterRoutingFunction(
	    loader, "pgr_bdAstar", 2, BindShortestPath<PathAlgorithm::BD_ASTAR, PathOutput::PATHS>, astar_options,
	    Doc(R"(
Shortest path(s) using a bidirectional A* search.

`pgr_bdAstar(edges, start vids, end vids, [directed := true, heuristic := 5, factor := 1, epsilon := 1])`)",
	        {EDGES_DOC, astar_doc, VIDS_DOC, PATH_RESULT_DOC, DEVIATION_DOC})
	        .c_str(),
	    R"(
SELECT * FROM pgr_bdAstar((SELECT id, source, target, cost, reverse_cost, x1, y1, x2, y2 FROM edges), 6, 10);
)");

	const char *const cost_result_doc = R"(
The result has one row per pair, ordered by `start_vid` and `end_vid`:

| Column | Type | Description |
| --- | --- | --- |
| `start_vid` | BIGINT | Identifier of the starting vertex |
| `end_vid` | BIGINT | Identifier of the ending vertex |
| `agg_cost` | DOUBLE | Cost of the shortest path from `start_vid` to `end_vid` |

Pairs without a path and pairs made of the same vertex twice are not returned.)";

	RegisterRoutingFunction(
	    loader, "pgr_dijkstraCost", 2, BindShortestPath<PathAlgorithm::DIJKSTRA, PathOutput::COSTS>, directed_option,
	    Doc(R"(
Cost of the shortest path(s) using Dijkstra's algorithm, without the paths themselves.

`pgr_dijkstraCost(edges, start vids, end vids, [directed := true])`)",
	        {EDGES_DOC, VIDS_DOC, cost_result_doc, DEVIATION_DOC})
	        .c_str(),
	    R"(
SELECT * FROM pgr_dijkstraCost((SELECT id, source, target, cost, reverse_cost FROM edges), [6, 1], [10, 17]);
)");

	RegisterRoutingFunction(
	    loader, "pgr_dijkstraCostMatrix", 1, BindShortestPath<PathAlgorithm::DIJKSTRA, PathOutput::COST_MATRIX>,
	    directed_option,
	    Doc(R"(
Cost matrix between a set of vertices using Dijkstra's algorithm. The result can be fed to `pgr_TSP`.

`pgr_dijkstraCostMatrix(edges, vids, [directed := true])`

`vids` is a constant list of vertex identifiers.)",
	        {EDGES_DOC, cost_result_doc, DEVIATION_DOC})
	        .c_str(),
	    R"(
SELECT * FROM pgr_dijkstraCostMatrix((SELECT id, source, target, cost, reverse_cost FROM edges), [5, 6, 10, 15], directed := false);
)");

	RegisterRoutingFunction(
	    loader, "pgr_drivingDistance", 2, BindDrivingDistance,
	    {{"directed", LogicalType::BOOLEAN}, {"equicost", LogicalType::BOOLEAN}},
	    Doc(R"(
Vertices whose shortest path cost from the root vertex is less than or equal to a distance, together with the shortest path tree that reaches them.

`pgr_drivingDistance(edges, root vids, distance, [directed := true, equicost := false])`

`root vids` is a single integer or a list of integers. When `equicost` is true a vertex is only reported for the root it is closest to (the smallest root identifier wins ties).)",
	        {EDGES_DOC, R"(
The result is ordered by `start_vid`, `depth` and `node`:

| Column | Type | Description |
| --- | --- | --- |
| `seq` | BIGINT | Sequential value starting from 1 |
| `depth` | BIGINT | Number of edges between `start_vid` and `node` in the shortest path tree |
| `start_vid` | BIGINT | Identifier of the root vertex |
| `pred` | BIGINT | Predecessor of `node` in the tree, the root itself for the root |
| `node` | BIGINT | Identifier of the reached vertex |
| `edge` | BIGINT | Identifier of the edge used to arrive to `node`, -1 for the root |
| `cost` | DOUBLE | Cost to traverse `edge` |
| `agg_cost` | DOUBLE | Aggregate cost from `start_vid` to `node` |)",
	         DEVIATION_DOC})
	        .c_str(),
	    R"(
SELECT * FROM pgr_drivingDistance((SELECT id, source, target, cost, reverse_cost FROM edges), 11, 3.0);

SELECT * FROM pgr_drivingDistance((SELECT id, source, target, cost, reverse_cost FROM edges), [11, 16], 3.0, equicost := true);
)");

	RegisterRoutingFunction(
	    loader, "pgr_KSP", 3, BindKShortestPath,
	    {{"directed", LogicalType::BOOLEAN}, {"heap_paths", LogicalType::BOOLEAN}},
	    Doc(R"(
K shortest loopless paths using Yen's algorithm.

`pgr_KSP(edges, start vids, end vids, K, [directed := true, heap_paths := false])`

At most `K` paths are returned per pair of start and end vertices, by increasing cost. When `heap_paths` is true the candidate paths that were found while searching are returned as well, after the K shortest ones.)",
	        {EDGES_DOC, VIDS_DOC, R"(
The result has the columns `seq`, `path_id`, `path_seq`, `start_vid`, `end_vid`, `node`, `edge`, `cost` and `agg_cost`. `path_id` numbers the paths from 1 across the whole result, the other columns are those of `pgr_dijkstra`.)",
	         DEVIATION_DOC})
	        .c_str(),
	    R"(
SELECT * FROM pgr_KSP((SELECT id, source, target, cost, reverse_cost FROM edges), 6, 17, 2);
)");
}

} // namespace routing

} // namespace duckdb
