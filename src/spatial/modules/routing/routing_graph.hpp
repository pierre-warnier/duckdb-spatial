#pragma once

#include "spatial/modules/routing/routing_common.hpp"

#include <limits>

namespace duckdb {

namespace routing {

static constexpr const uint32_t INVALID_VERTEX = std::numeric_limits<uint32_t>::max();
static constexpr const uint32_t INVALID_ARC = std::numeric_limits<uint32_t>::max();

inline double Infinity() {
	return std::numeric_limits<double>::infinity();
}

struct EdgeRecord {
	int64_t id;
	int64_t source;
	int64_t target;
	double cost;
	double reverse_cost;
};

struct Arc {
	uint32_t head;
	uint32_t edge;
	double cost;
};

struct PathStop {
	int64_t node;
	int64_t edge;
	double cost;
	double agg_cost;
};

struct Path {
	int64_t start_vid = 0;
	int64_t end_vid = 0;
	vector<PathStop> stops;
};

struct EdgeColumns {
	idx_t id;
	idx_t source;
	idx_t target;
	idx_t cost;
	idx_t reverse_cost;
};

//! Edges are sorted so that the result never depends on the order in which the rows arrive
vector<EdgeRecord> ReadEdges(const RoutingInput &input, const EdgeColumns &columns);
void SortEdges(vector<EdgeRecord> &edges);

class Graph {
public:
	Graph(vector<EdgeRecord> edges, bool directed);

	bool directed;
	vector<EdgeRecord> edges;
	vector<int64_t> vertex_ids;
	vector<uint32_t> out_offsets;
	vector<Arc> out_arcs;
	vector<uint32_t> in_offsets;
	vector<Arc> in_arcs;

	uint32_t VertexCount() const {
		return UnsafeNumericCast<uint32_t>(vertex_ids.size());
	}
	bool Lookup(int64_t id, uint32_t &index) const;
	vector<uint32_t> LookupAll(const vector<int64_t> &ids, vector<int64_t> &found_ids) const;
	void BuildReverse();

private:
	void CollectVertices();

	int64_t dense_base = 0;
	vector<uint32_t> dense_index;
};

struct HeapEntry {
	double key;
	uint32_t vertex;
};

struct HeapCompare {
	bool operator()(const HeapEntry &a, const HeapEntry &b) const {
		if (a.key != b.key) {
			return a.key > b.key;
		}
		return a.vertex > b.vertex;
	}
};

class MinHeap {
public:
	bool Empty() const {
		return entries.empty();
	}
	const HeapEntry &Top() const {
		return entries.front();
	}
	void Push(double key, uint32_t vertex);
	HeapEntry Pop();
	void Clear() {
		entries.clear();
	}

private:
	vector<HeapEntry> entries;
};

class SearchSpace {
public:
	explicit SearchSpace(uint32_t vertex_count);

	vector<double> dist;
	vector<uint32_t> pred_vertex;
	vector<uint32_t> pred_arc;
	vector<uint32_t> touched;
	MinHeap heap;

	void Reset();
	bool Reached(uint32_t vertex) const {
		return dist[vertex] != Infinity();
	}
	bool Relax(uint32_t vertex, double distance, uint32_t from, uint32_t arc) {
		if (distance < dist[vertex]) {
			if (dist[vertex] == Infinity()) {
				touched.push_back(vertex);
			}
			dist[vertex] = distance;
			pred_vertex[vertex] = from;
			pred_arc[vertex] = arc;
			return true;
		}
		return false;
	}
};

struct SearchOptions {
	double limit = Infinity();
	const vector<uint8_t> *targets = nullptr;
	idx_t target_count = 0;
	const vector<uint8_t> *blocked_vertices = nullptr;
	const vector<uint8_t> *blocked_arcs = nullptr;
};

void RunDijkstra(const vector<uint32_t> &offsets, const vector<Arc> &arcs, uint32_t source, SearchSpace &space,
                 const SearchOptions &options);

//! Path of a search that ran from `source` over the outgoing arcs
bool ForwardPath(const Graph &graph, const SearchSpace &space, uint32_t source, uint32_t target, Path &path);
//! Path of a search that ran from `target` over the incoming arcs
bool BackwardPath(const Graph &graph, const SearchSpace &space, uint32_t source, uint32_t target, Path &path);

void FinishPath(Path &path);

} // namespace routing

} // namespace duckdb
