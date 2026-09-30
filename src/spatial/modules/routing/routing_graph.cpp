#include "spatial/modules/routing/routing_graph.hpp"

#include "duckdb/common/exception.hpp"

#include <algorithm>

namespace duckdb {

namespace routing {

vector<EdgeRecord> ReadEdges(const RoutingInput &input, const EdgeColumns &columns) {
	vector<EdgeRecord> edges;
	edges.reserve(input.row_count);
	auto &ids = input.Get(columns.id).integers;
	auto &sources = input.Get(columns.source).integers;
	auto &targets = input.Get(columns.target).integers;
	auto &costs = input.Get(columns.cost).numerics;
	const auto has_reverse = input.Has(columns.reverse_cost);
	for (idx_t i = 0; i < input.row_count; i++) {
		EdgeRecord edge;
		edge.id = ids[i];
		edge.source = sources[i];
		edge.target = targets[i];
		edge.cost = costs[i];
		edge.reverse_cost = has_reverse ? input.Get(columns.reverse_cost).numerics[i] : -1;
		edges.push_back(edge);
	}
	SortEdges(edges);
	return edges;
}

void SortEdges(vector<EdgeRecord> &edges) {
	const auto less = [](const EdgeRecord &a, const EdgeRecord &b) {
		if (a.id != b.id) {
			return a.id < b.id;
		}
		if (a.source != b.source) {
			return a.source < b.source;
		}
		if (a.target != b.target) {
			return a.target < b.target;
		}
		if (a.cost != b.cost) {
			return a.cost < b.cost;
		}
		return a.reverse_cost < b.reverse_cost;
	};
	// The rows usually arrive ordered by identifier already
	if (!std::is_sorted(edges.begin(), edges.end(), less)) {
		std::sort(edges.begin(), edges.end(), less);
	}
}

static void BuildAdjacency(uint32_t vertex_count, const vector<uint32_t> &tails, const vector<Arc> &arcs,
                           vector<uint32_t> &offsets, vector<Arc> &result) {
	offsets.assign(static_cast<size_t>(vertex_count) + 1, 0);
	for (auto tail : tails) {
		offsets[tail + 1]++;
	}
	for (uint32_t v = 0; v < vertex_count; v++) {
		offsets[v + 1] += offsets[v];
	}
	result.resize(arcs.size());
	vector<uint32_t> cursor(offsets.begin(), offsets.end() - 1);
	for (idx_t i = 0; i < arcs.size(); i++) {
		result[cursor[tails[i]]++] = arcs[i];
	}
}

Graph::Graph(vector<EdgeRecord> edges_p, bool directed_p) : directed(directed_p), edges(std::move(edges_p)) {
	// Two vertices and two arcs per edge at most
	if (edges.size() >= INVALID_ARC / 2) {
		throw InvalidInputException("The graph is too large: at most %llu edges are supported",
		                            static_cast<idx_t>(INVALID_ARC / 2) - 1);
	}
	CollectVertices();

	vector<uint32_t> tails;
	vector<Arc> arcs;
	tails.reserve(edges.size() * 2);
	arcs.reserve(edges.size() * 2);
	for (idx_t i = 0; i < edges.size(); i++) {
		auto &edge = edges[i];
		uint32_t source = 0;
		uint32_t target = 0;
		Lookup(edge.source, source);
		Lookup(edge.target, target);
		const auto edge_idx = UnsafeNumericCast<uint32_t>(i);
		if (!directed) {
			// Both costs are usable both ways: only the cheapest one can be part of a shortest path
			auto cost = edge.cost >= 0 ? edge.cost : edge.reverse_cost;
			if (edge.reverse_cost >= 0 && edge.reverse_cost < cost) {
				cost = edge.reverse_cost;
			}
			if (cost >= 0) {
				tails.push_back(source);
				arcs.push_back(Arc {target, edge_idx, cost});
				tails.push_back(target);
				arcs.push_back(Arc {source, edge_idx, cost});
			}
			continue;
		}
		if (edge.cost >= 0) {
			tails.push_back(source);
			arcs.push_back(Arc {target, edge_idx, edge.cost});
		}
		if (edge.reverse_cost >= 0) {
			tails.push_back(target);
			arcs.push_back(Arc {source, edge_idx, edge.reverse_cost});
		}
	}
	BuildAdjacency(VertexCount(), tails, arcs, out_offsets, out_arcs);
}

void Graph::CollectVertices() {
	if (edges.empty()) {
		return;
	}
	auto min_id = edges[0].source;
	auto max_id = edges[0].source;
	for (auto &edge : edges) {
		min_id = MinValue(min_id, MinValue(edge.source, edge.target));
		max_id = MaxValue(max_id, MaxValue(edge.source, edge.target));
	}
	// Identifiers that are packed closely enough are mapped with a table instead of a binary search
	const auto range = static_cast<uint64_t>(max_id) - static_cast<uint64_t>(min_id);
	if (range <= 8 * static_cast<uint64_t>(edges.size()) + 1024) {
		dense_base = min_id;
		dense_index.assign(range + 1, INVALID_VERTEX);
		for (auto &edge : edges) {
			dense_index[static_cast<uint64_t>(edge.source) - static_cast<uint64_t>(min_id)] = 0;
			dense_index[static_cast<uint64_t>(edge.target) - static_cast<uint64_t>(min_id)] = 0;
		}
		for (uint64_t offset = 0; offset <= range; offset++) {
			if (dense_index[offset] != INVALID_VERTEX) {
				dense_index[offset] = UnsafeNumericCast<uint32_t>(vertex_ids.size());
				vertex_ids.push_back(static_cast<int64_t>(static_cast<uint64_t>(min_id) + offset));
			}
		}
		return;
	}
	vertex_ids.reserve(edges.size() * 2);
	for (auto &edge : edges) {
		vertex_ids.push_back(edge.source);
		vertex_ids.push_back(edge.target);
	}
	std::sort(vertex_ids.begin(), vertex_ids.end());
	vertex_ids.erase(std::unique(vertex_ids.begin(), vertex_ids.end()), vertex_ids.end());
}

bool Graph::Lookup(int64_t id, uint32_t &index) const {
	if (!dense_index.empty()) {
		const auto offset = static_cast<uint64_t>(id) - static_cast<uint64_t>(dense_base);
		if (id < dense_base || offset >= dense_index.size() || dense_index[offset] == INVALID_VERTEX) {
			return false;
		}
		index = dense_index[offset];
		return true;
	}
	auto entry = std::lower_bound(vertex_ids.begin(), vertex_ids.end(), id);
	if (entry == vertex_ids.end() || *entry != id) {
		return false;
	}
	index = UnsafeNumericCast<uint32_t>(entry - vertex_ids.begin());
	return true;
}

vector<uint32_t> Graph::LookupAll(const vector<int64_t> &ids, vector<int64_t> &found_ids) const {
	vector<uint32_t> result;
	found_ids.clear();
	for (auto id : ids) {
		uint32_t index;
		if (Lookup(id, index)) {
			result.push_back(index);
			found_ids.push_back(id);
		}
	}
	return result;
}

void Graph::BuildReverse() {
	if (!in_offsets.empty()) {
		return;
	}
	vector<uint32_t> tails;
	vector<Arc> arcs;
	tails.reserve(out_arcs.size());
	arcs.reserve(out_arcs.size());
	for (uint32_t v = 0; v < VertexCount(); v++) {
		for (auto i = out_offsets[v]; i < out_offsets[v + 1]; i++) {
			auto &arc = out_arcs[i];
			tails.push_back(arc.head);
			arcs.push_back(Arc {v, arc.edge, arc.cost});
		}
	}
	BuildAdjacency(VertexCount(), tails, arcs, in_offsets, in_arcs);
}

void MinHeap::Push(double key, uint32_t vertex) {
	entries.push_back(HeapEntry {key, vertex});
	std::push_heap(entries.begin(), entries.end(), HeapCompare());
}

HeapEntry MinHeap::Pop() {
	std::pop_heap(entries.begin(), entries.end(), HeapCompare());
	auto result = entries.back();
	entries.pop_back();
	return result;
}

SearchSpace::SearchSpace(uint32_t vertex_count)
    : dist(vertex_count, Infinity()), pred_vertex(vertex_count, INVALID_VERTEX), pred_arc(vertex_count, INVALID_ARC) {
}

void SearchSpace::Reset() {
	for (auto vertex : touched) {
		dist[vertex] = Infinity();
		pred_vertex[vertex] = INVALID_VERTEX;
		pred_arc[vertex] = INVALID_ARC;
	}
	touched.clear();
	heap.Clear();
}

void RunDijkstra(const vector<uint32_t> &offsets, const vector<Arc> &arcs, uint32_t source, SearchSpace &space,
                 const SearchOptions &options) {
	space.Reset();
	space.Relax(source, 0, INVALID_VERTEX, INVALID_ARC);
	space.heap.Push(0, source);
	auto remaining = options.target_count;

	while (!space.heap.Empty()) {
		const auto entry = space.heap.Pop();
		const auto vertex = entry.vertex;
		if (entry.key > space.dist[vertex]) {
			continue;
		}
		if (options.targets && (*options.targets)[vertex]) {
			remaining--;
			if (remaining == 0) {
				break;
			}
		}
		for (auto i = offsets[vertex]; i < offsets[vertex + 1]; i++) {
			auto &arc = arcs[i];
			if (options.blocked_arcs && (*options.blocked_arcs)[i]) {
				continue;
			}
			if (options.blocked_vertices && (*options.blocked_vertices)[arc.head]) {
				continue;
			}
			const auto distance = entry.key + arc.cost;
			if (distance > options.limit) {
				continue;
			}
			if (space.Relax(arc.head, distance, vertex, i)) {
				space.heap.Push(distance, arc.head);
			}
		}
	}
}

void FinishPath(Path &path) {
	double agg_cost = 0;
	for (auto &stop : path.stops) {
		stop.agg_cost = agg_cost;
		agg_cost += stop.cost;
	}
}

bool ForwardPath(const Graph &graph, const SearchSpace &space, uint32_t source, uint32_t target, Path &path) {
	if (!space.Reached(target) || source == target) {
		return false;
	}
	path.start_vid = graph.vertex_ids[source];
	path.end_vid = graph.vertex_ids[target];
	path.stops.clear();
	path.stops.push_back(PathStop {graph.vertex_ids[target], -1, 0, 0});
	auto vertex = target;
	while (vertex != source) {
		auto &arc = graph.out_arcs[space.pred_arc[vertex]];
		vertex = space.pred_vertex[vertex];
		path.stops.push_back(PathStop {graph.vertex_ids[vertex], graph.edges[arc.edge].id, arc.cost, 0});
	}
	std::reverse(path.stops.begin(), path.stops.end());
	FinishPath(path);
	return true;
}

bool BackwardPath(const Graph &graph, const SearchSpace &space, uint32_t source, uint32_t target, Path &path) {
	if (!space.Reached(source) || source == target) {
		return false;
	}
	path.start_vid = graph.vertex_ids[source];
	path.end_vid = graph.vertex_ids[target];
	path.stops.clear();
	auto vertex = source;
	while (vertex != target) {
		auto &arc = graph.in_arcs[space.pred_arc[vertex]];
		path.stops.push_back(PathStop {graph.vertex_ids[vertex], graph.edges[arc.edge].id, arc.cost, 0});
		vertex = space.pred_vertex[vertex];
	}
	path.stops.push_back(PathStop {graph.vertex_ids[target], -1, 0, 0});
	FinishPath(path);
	return true;
}

} // namespace routing

} // namespace duckdb
