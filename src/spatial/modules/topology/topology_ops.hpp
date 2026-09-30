#pragma once

#include "spatial/modules/topology/topology_session.hpp"

namespace duckdb {

namespace topology {

using TopoRows = vector<vector<Value>>;

TopoPoint ParsePoint(TopoSession &session, const Value &value);
vector<TopoPoint> ParseLine(TopoSession &session, const Value &value, GeosGeometry &line);
void RequireArguments(const vector<Value> &args);

// Editing (topology_edit.cpp)
int32_t AddIsoNode(TopoSession &session, const Value &face, const Value &point);
int32_t AddIsoEdge(TopoSession &session, int32_t start_node, int32_t end_node, const Value &line);
int32_t AddEdge(TopoSession &session, int32_t start_node, int32_t end_node, const Value &line, bool mod_face);
Value RemoveEdge(TopoSession &session, int32_t edge_id, bool mod_face);
void ChangeEdgeGeom(TopoSession &session, int32_t edge_id, const Value &line);
int32_t SplitEdge(TopoSession &session, int32_t edge_id, const Value &point, bool mod_edge);
int32_t HealEdges(TopoSession &session, int32_t edge1, int32_t edge2, bool mod_edge);
void MoveIsoNode(TopoSession &session, int32_t node_id, const Value &point);
void RemoveIsoNode(TopoSession &session, int32_t node_id);
void RemoveIsoEdge(TopoSession &session, int32_t edge_id);

// Population and validation (topology_build.cpp)
void CreateTopoGeo(TopoSession &session, const Value &collection);
void ValidateTopology(TopoSession &session, TopoRows &rows);

// Accessors (topology_query.cpp)
Value GetFaceGeometry(TopoSession &session, int32_t face_id);
vector<int32_t> GetFaceEdges(TopoSession &session, int32_t face_id);
int32_t GetNodeByPoint(TopoSession &session, const Value &point, double tolerance);
int32_t GetEdgeByPoint(TopoSession &session, const Value &point, double tolerance);
int32_t GetFaceByPoint(TopoSession &session, const Value &point, double tolerance);
vector<int32_t> GetNodeEdges(TopoSession &session, int32_t node_id);
vector<int32_t> GetRingEdges(TopoSession &session, int32_t edge_id, const Value &max_edges);
//! Polygon covered by a face, built from the edges that have the face on exactly one side
GeosGeometry BuildFaceGeometry(TopoSession &session, const vector<const TopoEdge *> &edges);

} // namespace topology

} // namespace duckdb
