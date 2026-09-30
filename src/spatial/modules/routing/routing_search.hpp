#pragma once

#include "spatial/modules/routing/routing_graph.hpp"

namespace duckdb {

namespace routing {

extern const vector<ColumnSpec> EDGE_COLUMN_SPECS;
extern const EdgeColumns EDGE_COLUMN_INDEXES;

vector<std::pair<const char *, LogicalType>> PathSchema();
void SortPaths(vector<Path> &paths);
void EmitPaths(RoutingResult &result, const vector<Path> &paths);

//! Shortest paths between every reachable (source, target) pair, ordered by (start_vid, end_vid)
vector<Path> ShortestPaths(ClientContext &context, Graph &graph, const vector<int64_t> &start_vids,
                           const vector<int64_t> &end_vids);

struct StructListArgument {
	vector<string> names;
	vector<LogicalType> types;
	vector<vector<Value>> rows;

	bool Find(const char *name, idx_t &index) const;
};

StructListArgument ParseStructList(RoutingBinder &binder, idx_t argument, const char *argument_name);

} // namespace routing

} // namespace duckdb
