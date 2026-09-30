#include "spatial/modules/topology/topology_ops.hpp"

#include "duckdb/common/string_util.hpp"
#include "duckdb/common/unordered_set.hpp"

namespace duckdb {

namespace topology {

//------------------------------------------------------------------------------
// Argument parsing
//------------------------------------------------------------------------------

void RequireArguments(const vector<Value> &args) {
	for (auto &arg : args) {
		if (arg.IsNull()) {
			throw InvalidInputException("SQL/MM Spatial exception - null argument");
		}
	}
}

TopoPoint ParsePoint(TopoSession &session, const Value &value) {
	const auto geom = session.ToGeos(value);
	if (geom.type() != GEOS_POINT) {
		throw InvalidInputException("SQL/MM Spatial exception - invalid point");
	}
	const auto pts = session.Coordinates(geom.get_raw());
	if (pts.empty()) {
		throw InvalidInputException("Cannot use an empty point");
	}
	return pts[0];
}

vector<TopoPoint> ParseLine(TopoSession &session, const Value &value, GeosGeometry &line) {
	const auto geom = session.ToGeos(value);
	if (geom.type() != GEOS_LINESTRING) {
		throw InvalidInputException("SQL/MM Spatial exception - invalid curve");
	}
	auto pts = session.Coordinates(geom.get_raw());
	bool distinct = false;
	for (idx_t i = 1; i < pts.size(); i++) {
		distinct = distinct || pts[i] != pts[0];
	}
	if (!distinct) {
		throw InvalidInputException("Invalid edge (no two distinct vertices exist)");
	}
	line = session.MakeLine(pts);
	if (!line.is_simple()) {
		throw InvalidInputException("SQL/MM Spatial exception - curve not simple");
	}
	return pts;
}

//------------------------------------------------------------------------------
// Editor: the edges around an edit, changed in memory and written back at once
//------------------------------------------------------------------------------

namespace {

class TopoEditor {
public:
	explicit TopoEditor(TopoSession &session_p) : session(session_p) {
	}

	void Add(vector<TopoEdge> loaded) {
		for (auto &edge : loaded) {
			if (edges.find(edge.id) != edges.end() || removed.find(edge.id) != removed.end()) {
				continue;
			}
			original[edge.id] = edge;
			edges[edge.id] = std::move(edge);
		}
	}

	void LoadAtNodes(int32_t a, int32_t b) {
		Add(session.LoadEdges("start_node IN (?, ?) OR end_node IN (?, ?)",
		                      {Value::INTEGER(a), Value::INTEGER(b), Value::INTEGER(a), Value::INTEGER(b)}));
	}

	void LoadOfFace(int32_t face) {
		Add(session.LoadEdges("left_face = ? OR right_face = ?", {Value::INTEGER(face), Value::INTEGER(face)}));
	}

	TopoEdge *Find(int32_t id) {
		auto entry = edges.find(id);
		if (entry != edges.end()) {
			return &entry->second;
		}
		if (removed.find(id) != removed.end()) {
			return nullptr;
		}
		TopoEdge edge;
		if (!session.LoadEdge(id, edge)) {
			return nullptr;
		}
		original[id] = edge;
		return &(edges[id] = std::move(edge));
	}

	TopoEdge &Create(TopoEdge edge) {
		created.insert(edge.id);
		const auto id = edge.id;
		return edges[id] = std::move(edge);
	}

	void Remove(int32_t id) {
		edges.erase(id);
		removed.insert(id);
	}

	vector<EdgeEnd> Fan(int32_t node) const {
		vector<EdgeEnd> fan;
		for (auto &entry : edges) {
			auto &edge = entry.second;
			if (edge.start_node == node) {
				fan.push_back(EdgeEnd {edge.id, true, EdgeEndAngle(edge, true)});
			}
			if (edge.end_node == node) {
				fan.push_back(EdgeEnd {edge.id, false, EdgeEndAngle(edge, false)});
			}
		}
		SortFan(fan);
		return fan;
	}

	//! Recompute next_left_edge / next_right_edge of every edge end attached to the node. All edges incident to the
	//! node must have been loaded. Returns the number of edge ends found
	idx_t Relink(int32_t node) {
		const auto fan = Fan(node);
		LinkFan(fan, [&](int32_t edge_id, bool is_left, int32_t value) {
			auto &edge = edges[edge_id];
			if (is_left) {
				edge.next_left = value;
			} else {
				edge.next_right = value;
			}
		});
		return fan.size();
	}

	vector<int32_t> Ring(int32_t start) {
		vector<int32_t> ring;
		const auto closed = WalkRing([&](int32_t id) { return Find(id); }, start, RING_LIMIT, ring);
		if (!closed) {
			throw InvalidInputException("Corrupted topology: ring of edge %d is not closed", start);
		}
		return ring;
	}

	vector<TopoPoint> RingCoords(const vector<int32_t> &ring) {
		return RingCoordinates([&](int32_t id) { return Find(id); }, ring);
	}

	void Flush() {
		vector<int32_t> deleted;
		for (auto id : removed) {
			if (original.find(id) != original.end()) {
				deleted.push_back(id);
			}
		}
		if (!deleted.empty()) {
			session.Query("DELETE FROM " + session.Table("edge_data") + " WHERE list_contains(?, edge_id)",
			              {TopoSession::IdList(deleted)});
		}
		vector<int32_t> ids;
		vector<int32_t> next_left;
		vector<int32_t> next_right;
		vector<int32_t> left_face;
		vector<int32_t> right_face;
		for (auto &entry : edges) {
			auto &edge = entry.second;
			if (created.find(edge.id) != created.end()) {
				session.Query("INSERT INTO " + session.Table("edge_data") +
				                  "(edge_id, start_node, end_node, next_left_edge, abs_next_left_edge, "
				                  "next_right_edge, abs_next_right_edge, left_face, right_face, geom) "
				                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
				              {Value::INTEGER(edge.id), Value::INTEGER(edge.start_node), Value::INTEGER(edge.end_node),
				               Value::INTEGER(edge.next_left), Value::INTEGER(AbsValue(edge.next_left)),
				               Value::INTEGER(edge.next_right), Value::INTEGER(AbsValue(edge.next_right)),
				               Value::INTEGER(edge.left_face), Value::INTEGER(edge.right_face),
				               session.LineValue(edge.pts)});
				continue;
			}
			auto &before = original[edge.id];
			if (before.start_node != edge.start_node || before.end_node != edge.end_node || before.pts != edge.pts) {
				session.Query("UPDATE " + session.Table("edge_data") +
				                  " SET start_node = ?, end_node = ?, geom = ? WHERE edge_id = ?",
				              {Value::INTEGER(edge.start_node), Value::INTEGER(edge.end_node),
				               session.LineValue(edge.pts), Value::INTEGER(edge.id)});
			}
			if (!before.SameLinks(edge)) {
				ids.push_back(edge.id);
				next_left.push_back(edge.next_left);
				next_right.push_back(edge.next_right);
				left_face.push_back(edge.left_face);
				right_face.push_back(edge.right_face);
			}
		}
		if (!ids.empty()) {
			session.Query(
			    "UPDATE " + session.Table("edge_data") +
			        " SET next_left_edge = u.nl, abs_next_left_edge = abs(u.nl), next_right_edge = u.nr, "
			        "abs_next_right_edge = abs(u.nr), left_face = u.lf, right_face = u.rf "
			        "FROM (SELECT unnest(?::INTEGER[]) AS id, unnest(?::INTEGER[]) AS nl, unnest(?::INTEGER[]) AS nr, "
			        "unnest(?::INTEGER[]) AS lf, unnest(?::INTEGER[]) AS rf) u WHERE edge_id = u.id",
			    {TopoSession::IdList(ids), TopoSession::IdList(next_left), TopoSession::IdList(next_right),
			     TopoSession::IdList(left_face), TopoSession::IdList(right_face)});
		}
		original = edges;
		created.clear();
		removed.clear();
	}

	TopoSession &session;
	EdgeMap edges;

private:
	static constexpr idx_t RING_LIMIT = 100000000;

	EdgeMap original;
	unordered_set<int32_t> created;
	unordered_set<int32_t> removed;
};

} // namespace

//------------------------------------------------------------------------------
// Shared checks
//------------------------------------------------------------------------------

static void CheckEdgeCrossing(TopoSession &session, const GeosGeometry &line, const vector<TopoPoint> &pts,
                              int32_t start_node, int32_t end_node, int32_t ignored_edge) {
	TopoBox box;
	box.Add(pts);
	for (auto &node : session.LoadNodesInBox(box)) {
		if (node.id == start_node || node.id == end_node) {
			continue;
		}
		idx_t segment;
		bool at_vertex;
		if (LocatePointOnLine(pts, node.pt, segment, at_vertex)) {
			throw InvalidInputException("SQL/MM Spatial exception - geometry crosses a node");
		}
	}
	const auto ctx = session.Geos();
	for (auto &edge : session.LoadEdgesInBox(box)) {
		if (edge.id == ignored_edge) {
			continue;
		}
		const auto other = session.MakeLine(edge.pts);
		const auto matrix = GEOSRelateBoundaryNodeRule_r(ctx, other.get_raw(), line.get_raw(), GEOSRELATE_BNR_ENDPOINT);
		if (!matrix) {
			throw InvalidInputException("Could not relate the geometry with edge %d", edge.id);
		}
		const string relation(matrix);
		GEOSFree_r(ctx, matrix);
		auto matches = [&](const char *pattern) {
			return GEOSRelatePatternMatch_r(ctx, relation.c_str(), pattern) == 1;
		};
		if (matches("FF*F*****")) {
			continue;
		}
		if (matches("1FFF*FFF2")) {
			throw InvalidInputException("SQL/MM Spatial exception - coincident edge %d", edge.id);
		}
		if (matches("1********")) {
			throw InvalidInputException("Spatial exception - geometry intersects edge %d", edge.id);
		}
		if (matches("T********")) {
			throw InvalidInputException("SQL/MM Spatial exception - geometry crosses edge %d", edge.id);
		}
		if (matches("*T*******")) {
			throw InvalidInputException("Spatial exception - geometry boundary touches interior of edge %d", edge.id);
		}
		throw InvalidInputException("Spatial exception - boundary of edge %d touches interior of geometry", edge.id);
	}
}

static void CheckEndpoints(const vector<TopoPoint> &pts, const TopoNode &start, const TopoNode &end) {
	if (pts.front() != start.pt) {
		throw InvalidInputException("SQL/MM Spatial exception - start node not geometry start point.");
	}
	if (pts.back() != end.pt) {
		throw InvalidInputException("SQL/MM Spatial exception - end node not geometry end point.");
	}
}

//! Face of the sector around a node into which an edge end leaving with the given angle falls
static int32_t WedgeFace(TopoEditor &editor, const TopoNode &node, double angle) {
	const auto fan = editor.Fan(node.id);
	if (fan.empty()) {
		if (!node.has_face) {
			throw InvalidInputException("Corrupted topology: node %d has no edges and no containing face", node.id);
		}
		return node.face;
	}
	idx_t ccw = 0;
	while (ccw < fan.size() && fan[ccw].angle <= angle) {
		ccw++;
	}
	auto &next = fan[ccw % fan.size()];
	auto &prev = fan[(ccw + fan.size() - 1) % fan.size()];
	auto &prev_edge = editor.edges[prev.edge];
	auto &next_edge = editor.edges[next.edge];
	const auto cw_face = prev.outgoing ? prev_edge.left_face : prev_edge.right_face;
	const auto ccw_face = next.outgoing ? next_edge.right_face : next_edge.left_face;
	if (cw_face != ccw_face) {
		throw InvalidInputException("Corrupted topology: adjacent edges %d and %d bind different faces (%d and %d)",
		                            prev.Signed(), next.Signed(), cw_face, ccw_face);
	}
	return cw_face;
}

//------------------------------------------------------------------------------
// Isolated nodes and edges
//------------------------------------------------------------------------------

int32_t AddIsoNode(TopoSession &session, const Value &face, const Value &point) {
	const auto p = ParsePoint(session, point);
	session.CheckNodeLocation(p);
	const auto containing = session.FaceContainingPoint(p);
	if (!face.IsNull() && face.GetValue<int32_t>() != containing) {
		throw InvalidInputException("SQL/MM Spatial exception - not within face");
	}
	TopoNode node;
	node.id = session.NextId("node_node_id_seq");
	node.has_face = true;
	node.face = containing;
	node.pt = p;
	session.InsertNode(node);
	return node.id;
}

void MoveIsoNode(TopoSession &session, int32_t node_id, const Value &point) {
	const auto p = ParsePoint(session, point);
	TopoNode node;
	if (!session.LoadNode(node_id, node)) {
		throw InvalidInputException("SQL/MM Spatial exception - non-existent node");
	}
	if (!node.has_face) {
		throw InvalidInputException("SQL/MM Spatial exception - not isolated node");
	}
	session.CheckNodeLocation(p);
	if (session.FaceContainingPoint(p) != node.face) {
		throw InvalidInputException("Cannot move isolated node across faces");
	}
	session.Query("UPDATE " + session.Table("node") + " SET geom = ? WHERE node_id = ?",
	              {session.PointValue(p), Value::INTEGER(node_id)});
}

void RemoveIsoNode(TopoSession &session, int32_t node_id) {
	TopoNode node;
	if (!session.LoadNode(node_id, node)) {
		throw InvalidInputException("SQL/MM Spatial exception - non-existent node");
	}
	const auto edges =
	    session.LoadEdges("start_node = ? OR end_node = ?", {Value::INTEGER(node_id), Value::INTEGER(node_id)});
	if (!node.has_face || !edges.empty()) {
		throw InvalidInputException("SQL/MM Spatial exception - not isolated node");
	}
	session.Query("DELETE FROM " + session.Table("node") + " WHERE node_id = ?", {Value::INTEGER(node_id)});
}

int32_t AddIsoEdge(TopoSession &session, int32_t start_node, int32_t end_node, const Value &line_value) {
	if (start_node == end_node) {
		throw InvalidInputException("Closed edges would not be isolated, try ST_AddEdgeNewFaces");
	}
	GeosGeometry line(session.Geos(), nullptr);
	const auto pts = ParseLine(session, line_value, line);
	TopoNode start;
	TopoNode end;
	if (!session.LoadNode(start_node, start) || !session.LoadNode(end_node, end)) {
		throw InvalidInputException("SQL/MM Spatial exception - non-existent node");
	}
	if (!start.has_face || !end.has_face) {
		throw InvalidInputException("SQL/MM Spatial exception - not isolated node");
	}
	if (start.face != end.face) {
		throw InvalidInputException("SQL/MM Spatial exception - nodes in different faces");
	}
	CheckEndpoints(pts, start, end);
	CheckEdgeCrossing(session, line, pts, start_node, end_node, 0);

	TopoEditor editor(session);
	TopoEdge edge;
	edge.id = session.NextId("edge_data_edge_id_seq");
	edge.start_node = start_node;
	edge.end_node = end_node;
	edge.next_left = -edge.id;
	edge.next_right = edge.id;
	edge.left_face = start.face;
	edge.right_face = start.face;
	edge.pts = pts;
	editor.Create(edge);
	editor.Flush();
	session.SetContainingFace({start_node, end_node}, Value());
	return edge.id;
}

void RemoveIsoEdge(TopoSession &session, int32_t edge_id) {
	TopoEditor editor(session);
	const auto found = editor.Find(edge_id);
	if (!found) {
		throw InvalidInputException("SQL/MM Spatial exception - non-existent edge");
	}
	const auto edge = *found;
	editor.LoadAtNodes(edge.start_node, edge.end_node);
	if (edge.left_face != edge.right_face || editor.edges.size() != 1 || edge.IsClosed()) {
		throw InvalidInputException("SQL/MM Spatial exception - not isolated edge");
	}
	editor.Remove(edge_id);
	editor.Flush();
	session.SetContainingFace({edge.start_node, edge.end_node}, Value::INTEGER(edge.left_face));
}

//------------------------------------------------------------------------------
// ST_AddEdgeNewFaces / ST_AddEdgeModFace
//------------------------------------------------------------------------------

//! Register the face found on the left of the ring starting at sedge, which used to be part of `face`.
//! Returns the new face id, 0 if the edge does not close a ring, -1 if no face was created on that side
static int32_t AddFaceSplit(TopoSession &session, TopoEditor &editor, int32_t sedge, int32_t face, bool mbr_only) {
	const auto ring = editor.Ring(sedge);
	for (auto signed_id : ring) {
		if (signed_id == -sedge) {
			return 0;
		}
	}
	const auto coords = editor.RingCoords(ring);
	const bool is_ccw = SignedArea(coords) > 0;
	TopoBox shell_box;
	shell_box.Add(coords);

	if (face == 0 && !is_ccw) {
		return -1;
	}
	if (mbr_only && face != 0) {
		if (is_ccw) {
			session.UpdateFaceBox(face, shell_box);
		}
		return -1;
	}

	auto mbr = shell_box;
	if (face != 0 && !is_ccw) {
		// The ring is a hole of the new face, which keeps the extent of the face being split
		TopoFace old_face;
		if (!session.LoadFace(face, old_face)) {
			throw InvalidInputException("Corrupted topology: face %d is not registered", face);
		}
		mbr = old_face.mbr;
	}
	const auto new_face = session.NextId("face_face_id_seq");
	session.InsertFace(new_face, mbr);

	unordered_set<int32_t> ring_edges;
	for (auto signed_id : ring) {
		auto &edge = *editor.Find(AbsValue(signed_id));
		auto &side = signed_id > 0 ? edge.left_face : edge.right_face;
		if (side != face) {
			throw InvalidInputException("Corrupted topology: edge %d is on a ring of face %d but is labelled %d",
			                            signed_id, face, side);
		}
		side = new_face;
		ring_edges.insert(edge.id);
	}

	// Everything else that was in the split face goes to the new one when it lies on the new face's side of the ring
	auto belongs = [&](const TopoPoint &p) {
		const bool inside = shell_box.Contains(p) && PointInRing(coords, p);
		return inside == is_ccw;
	};
	for (auto &entry : editor.edges) {
		auto &edge = entry.second;
		if (ring_edges.find(edge.id) != ring_edges.end()) {
			continue;
		}
		if (edge.left_face != face && edge.right_face != face) {
			continue;
		}
		if (!belongs(InteriorPoint(edge.pts))) {
			continue;
		}
		if (edge.left_face == face) {
			edge.left_face = new_face;
		}
		if (edge.right_face == face) {
			edge.right_face = new_face;
		}
	}
	vector<int32_t> moved_nodes;
	for (auto &node : session.LoadNodes("containing_face = ?", {Value::INTEGER(face)})) {
		if (belongs(node.pt)) {
			moved_nodes.push_back(node.id);
		}
	}
	session.SetContainingFace(moved_nodes, Value::INTEGER(new_face));
	return new_face;
}

int32_t AddEdge(TopoSession &session, int32_t start_node, int32_t end_node, const Value &line_value, bool mod_face) {
	GeosGeometry line(session.Geos(), nullptr);
	const auto pts = ParseLine(session, line_value, line);
	TopoNode start;
	TopoNode end;
	if (!session.LoadNode(start_node, start) || !session.LoadNode(end_node, end)) {
		throw InvalidInputException("SQL/MM Spatial exception - non-existent node");
	}
	CheckEndpoints(pts, start, end);
	CheckEdgeCrossing(session, line, pts, start_node, end_node, 0);

	TopoEditor editor(session);
	editor.LoadAtNodes(start_node, end_node);

	TopoEdge edge;
	edge.start_node = start_node;
	edge.end_node = end_node;
	edge.pts = pts;
	const bool closed = edge.IsClosed();
	const bool start_isolated = editor.Fan(start_node).empty();
	const bool end_isolated = editor.Fan(end_node).empty();
	const auto start_face = WedgeFace(editor, start, EdgeEndAngle(edge, true));
	const auto end_face = WedgeFace(editor, end, EdgeEndAngle(edge, false));
	if (start_face != end_face) {
		throw InvalidInputException("SQL/MM Spatial exception - geometry crosses an edge (endnodes in faces %d and %d)",
		                            start_face, end_face);
	}
	const auto face = start_face;
	edge.id = session.NextId("edge_data_edge_id_seq");
	edge.left_face = face;
	edge.right_face = face;
	const auto id = edge.id;
	editor.Create(std::move(edge));
	editor.Relink(start_node);
	if (!closed) {
		editor.Relink(end_node);
	}

	vector<int32_t> connected;
	if (start_isolated) {
		connected.push_back(start_node);
	}
	if (end_isolated && !closed) {
		connected.push_back(end_node);
	}
	session.SetContainingFace(connected, Value());

	// A dangling edge cannot close a ring
	if (closed || (!start_isolated && !end_isolated)) {
		editor.LoadOfFace(face);
		if (mod_face) {
			const auto left = AddFaceSplit(session, editor, id, face, false);
			if (left < 0) {
				AddFaceSplit(session, editor, -id, face, false);
			} else if (left > 0) {
				AddFaceSplit(session, editor, -id, face, true);
			}
		} else {
			const auto right = AddFaceSplit(session, editor, -id, face, false);
			if (right != 0) {
				AddFaceSplit(session, editor, id, face, false);
				if (face != 0) {
					editor.Flush();
					session.Query("DELETE FROM " + session.Table("face") + " WHERE face_id = ?",
					              {Value::INTEGER(face)});
				}
			}
		}
	}
	editor.Flush();
	return id;
}

//------------------------------------------------------------------------------
// ST_RemEdgeNewFace / ST_RemEdgeModFace
//------------------------------------------------------------------------------

Value RemoveEdge(TopoSession &session, int32_t edge_id, bool mod_face) {
	TopoEditor editor(session);
	const auto found = editor.Find(edge_id);
	if (!found) {
		throw InvalidInputException("SQL/MM Spatial exception - non-existent edge %d", edge_id);
	}
	const auto edge = *found;
	editor.LoadAtNodes(edge.start_node, edge.end_node);
	editor.Remove(edge_id);
	vector<int32_t> isolated;
	if (editor.Relink(edge.start_node) == 0) {
		isolated.push_back(edge.start_node);
	}
	if (!edge.IsClosed() && editor.Relink(edge.end_node) == 0) {
		isolated.push_back(edge.end_node);
	}
	editor.Flush();

	auto flood_face = edge.left_face;
	Value new_face(LogicalType::INTEGER);
	if (edge.left_face != edge.right_face) {
		if (edge.left_face == 0 || edge.right_face == 0) {
			flood_face = 0;
		} else {
			TopoFace left;
			TopoFace right;
			if (!session.LoadFace(edge.left_face, left) || !session.LoadFace(edge.right_face, right)) {
				throw InvalidInputException("Corrupted topology: faces %d and %d of edge %d are not both registered",
				                            edge.left_face, edge.right_face, edge_id);
			}
			auto box = left.mbr;
			box.Merge(right.mbr);
			if (mod_face) {
				flood_face = edge.right_face;
				session.UpdateFaceBox(flood_face, box);
			} else {
				flood_face = session.NextId("face_face_id_seq");
				session.InsertFace(flood_face, box);
				new_face = Value::INTEGER(flood_face);
			}
		}
		const vector<Value> params = {Value::INTEGER(flood_face), Value::INTEGER(edge.left_face),
		                              Value::INTEGER(edge.right_face)};
		session.Query("UPDATE " + session.Table("edge_data") + " SET left_face = ? WHERE left_face IN (?, ?)", params);
		session.Query("UPDATE " + session.Table("edge_data") + " SET right_face = ? WHERE right_face IN (?, ?)",
		              params);
		session.Query("UPDATE " + session.Table("node") + " SET containing_face = ? WHERE containing_face IN (?, ?)",
		              params);
		session.Query("DELETE FROM " + session.Table("face") +
		                  " WHERE face_id <> ? AND face_id <> 0 AND face_id IN (?, ?)",
		              params);
	}
	session.SetContainingFace(isolated, Value::INTEGER(flood_face));
	return mod_face ? Value::INTEGER(flood_face) : new_face;
}

//------------------------------------------------------------------------------
// ST_ChangeEdgeGeom
//------------------------------------------------------------------------------

static vector<int32_t> CyclicOrder(const vector<EdgeEnd> &fan) {
	vector<int32_t> order;
	idx_t first = 0;
	for (idx_t i = 0; i < fan.size(); i++) {
		if (fan[i].Signed() < fan[first].Signed()) {
			first = i;
		}
	}
	for (idx_t i = 0; i < fan.size(); i++) {
		order.push_back(fan[(first + i) % fan.size()].Signed());
	}
	return order;
}

void ChangeEdgeGeom(TopoSession &session, int32_t edge_id, const Value &line_value) {
	GeosGeometry line(session.Geos(), nullptr);
	const auto pts = ParseLine(session, line_value, line);

	TopoEditor editor(session);
	const auto found = editor.Find(edge_id);
	if (!found) {
		throw InvalidInputException("SQL/MM Spatial exception - non-existent edge %d", edge_id);
	}
	auto &edge = *found;
	const auto old_pts = edge.pts;
	if (pts.front() != old_pts.front()) {
		throw InvalidInputException("SQL/MM Spatial exception - start node not geometry start point.");
	}
	if (pts.back() != old_pts.back()) {
		throw InvalidInputException("SQL/MM Spatial exception - end node not geometry end point.");
	}
	const bool closed = edge.IsClosed();
	if (closed && (SignedArea(old_pts) > 0) != (SignedArea(pts) > 0)) {
		throw InvalidInputException("Edge twist at node POINT(%s %s)", FormatNumber(pts[0].x), FormatNumber(pts[0].y));
	}
	CheckEdgeCrossing(session, line, pts, edge.start_node, edge.end_node, edge_id);

	// No node may lie in the area swept between the old and the new geometry
	TopoBox motion_box;
	motion_box.Add(old_pts);
	motion_box.Add(pts);
	for (auto &node : session.LoadNodesInBox(motion_box)) {
		if (node.id == edge.start_node || node.id == edge.end_node) {
			continue;
		}
		if (((CrossingCount(old_pts, node.pt) + CrossingCount(pts, node.pt)) & 1) == 1) {
			throw InvalidInputException("Edge motion collision at POINT(%s %s)", FormatNumber(node.pt.x),
			                            FormatNumber(node.pt.y));
		}
	}

	editor.LoadAtNodes(edge.start_node, edge.end_node);
	const bool two_faces = edge.left_face != edge.right_face;
	const auto ring_start = edge.left_face != 0 ? edge_id : -edge_id;
	bool ring_ccw = false;
	vector<int32_t> ring;
	if (two_faces) {
		editor.LoadOfFace(edge.left_face != 0 ? edge.left_face : edge.right_face);
		ring = editor.Ring(ring_start);
		ring_ccw = SignedArea(editor.RingCoords(ring)) > 0;
	}
	const auto start_order = CyclicOrder(editor.Fan(edge.start_node));
	const auto end_order = CyclicOrder(editor.Fan(edge.end_node));

	edge.pts = pts;

	if (CyclicOrder(editor.Fan(edge.start_node)) != start_order) {
		throw InvalidInputException("Edge changed disposition around start node %d", edge.start_node);
	}
	if (CyclicOrder(editor.Fan(edge.end_node)) != end_order) {
		throw InvalidInputException("Edge changed disposition around end node %d", edge.end_node);
	}
	if (two_faces && (SignedArea(editor.RingCoords(ring)) > 0) != ring_ccw) {
		throw InvalidInputException("Edge ring changes winding");
	}
	const auto left_face = edge.left_face;
	const auto right_face = edge.right_face;
	editor.Flush();

	TopoBox old_box;
	TopoBox new_box;
	old_box.Add(old_pts);
	new_box.Add(pts);
	if (!(old_box == new_box)) {
		session.RefreshFaceBox(left_face);
		if (right_face != left_face) {
			session.RefreshFaceBox(right_face);
		}
	}
}

//------------------------------------------------------------------------------
// ST_ModEdgeSplit / ST_NewEdgesSplit
//------------------------------------------------------------------------------

int32_t SplitEdge(TopoSession &session, int32_t edge_id, const Value &point, bool mod_edge) {
	TopoEditor editor(session);
	const auto found = editor.Find(edge_id);
	if (!found) {
		throw InvalidInputException("SQL/MM Spatial exception - non-existent edge");
	}
	auto &edge = *found;
	const auto p = ParsePoint(session, point);
	TopoBox box;
	box.Add(p);
	for (auto &node : session.LoadNodesInBox(box)) {
		if (node.pt == p) {
			throw InvalidInputException("SQL/MM Spatial exception - coincident node");
		}
	}
	idx_t segment;
	bool at_vertex;
	if (!LocatePointOnLine(edge.pts, p, segment, at_vertex)) {
		throw InvalidInputException("SQL/MM Spatial exception - point not on edge");
	}
	vector<TopoPoint> first(edge.pts.begin(), edge.pts.begin() + NumericCast<int64_t>(segment) + 1);
	vector<TopoPoint> second;
	if (!at_vertex) {
		first.push_back(p);
		second.push_back(p);
		second.insert(second.end(), edge.pts.begin() + NumericCast<int64_t>(segment) + 1, edge.pts.end());
	} else {
		second.assign(edge.pts.begin() + NumericCast<int64_t>(segment), edge.pts.end());
	}
	if (first.size() < 2 || second.size() < 2) {
		throw InvalidInputException("SQL/MM Spatial exception - coincident node");
	}

	TopoNode node;
	node.id = session.NextId("node_node_id_seq");
	node.pt = p;
	session.InsertNode(node);

	const auto start_node = edge.start_node;
	const auto end_node = edge.end_node;
	editor.LoadAtNodes(start_node, end_node);

	TopoEdge tail;
	tail.start_node = node.id;
	tail.end_node = end_node;
	tail.left_face = edge.left_face;
	tail.right_face = edge.right_face;
	tail.pts = std::move(second);
	if (mod_edge) {
		tail.id = session.NextId("edge_data_edge_id_seq");
		edge.end_node = node.id;
		edge.pts = std::move(first);
		editor.Create(std::move(tail));
	} else {
		const auto ids = session.NextIds("edge_data_edge_id_seq", 2);
		TopoEdge head;
		head.id = ids[0];
		head.start_node = start_node;
		head.end_node = node.id;
		head.left_face = edge.left_face;
		head.right_face = edge.right_face;
		head.pts = std::move(first);
		tail.id = ids[1];
		editor.Remove(edge_id);
		editor.Create(std::move(head));
		editor.Create(std::move(tail));
	}
	editor.Relink(start_node);
	editor.Relink(node.id);
	if (end_node != start_node) {
		editor.Relink(end_node);
	}
	editor.Flush();
	return node.id;
}

//------------------------------------------------------------------------------
// ST_ModEdgeHeal / ST_NewEdgeHeal
//------------------------------------------------------------------------------

static void AppendPoints(vector<TopoPoint> &target, const vector<TopoPoint> &source, bool reversed) {
	for (idx_t i = 0; i < source.size(); i++) {
		auto &p = reversed ? source[source.size() - 1 - i] : source[i];
		if (target.empty() || target.back() != p) {
			target.push_back(p);
		}
	}
}

int32_t HealEdges(TopoSession &session, int32_t edge1, int32_t edge2, bool mod_edge) {
	if (edge1 == edge2) {
		throw InvalidInputException("Cannot heal edge %d with itself, try with another", edge1);
	}
	TopoEditor editor(session);
	const auto found1 = editor.Find(edge1);
	if (!found1) {
		throw InvalidInputException("SQL/MM Spatial exception - non-existent edge %d", edge1);
	}
	const auto found2 = editor.Find(edge2);
	if (!found2) {
		throw InvalidInputException("SQL/MM Spatial exception - non-existent edge %d", edge2);
	}
	const auto e1 = *found1;
	const auto e2 = *found2;
	if (e1.IsClosed()) {
		throw InvalidInputException("Edge %d is closed, cannot heal to edge %d", edge1, edge2);
	}
	if (e2.IsClosed()) {
		throw InvalidInputException("Edge %d is closed, cannot heal to edge %d", edge2, edge1);
	}
	editor.LoadAtNodes(e1.start_node, e1.end_node);

	string others;
	auto is_free = [&](int32_t node) {
		bool free = true;
		for (auto &end : editor.Fan(node)) {
			if (end.edge != edge1 && end.edge != edge2) {
				free = false;
				others += (others.empty() ? "" : ",") + to_string(end.edge);
			}
		}
		return free;
	};
	int32_t common_node = -1;
	idx_t layout = 0;
	if (e1.end_node == e2.start_node) {
		common_node = e1.end_node;
		layout = 1;
	} else if (e1.end_node == e2.end_node) {
		common_node = e1.end_node;
		layout = 2;
	}
	if (common_node != -1 && !is_free(common_node)) {
		common_node = -1;
	}
	if (common_node == -1) {
		if (e1.start_node == e2.start_node) {
			common_node = e1.start_node;
			layout = 3;
		} else if (e1.start_node == e2.end_node) {
			common_node = e1.start_node;
			layout = 4;
		}
		if (common_node != -1 && !is_free(common_node)) {
			common_node = -1;
		}
	}
	if (common_node == -1) {
		if (!others.empty()) {
			throw InvalidInputException("SQL/MM Spatial exception - other edges connected (%s)", others);
		}
		throw InvalidInputException("SQL/MM Spatial exception - non-connected edges");
	}

	TopoEdge healed;
	healed.left_face = e1.left_face;
	healed.right_face = e1.right_face;
	switch (layout) {
	case 1:
		AppendPoints(healed.pts, e1.pts, false);
		AppendPoints(healed.pts, e2.pts, false);
		healed.start_node = e1.start_node;
		healed.end_node = e2.end_node;
		break;
	case 2:
		AppendPoints(healed.pts, e1.pts, false);
		AppendPoints(healed.pts, e2.pts, true);
		healed.start_node = e1.start_node;
		healed.end_node = e2.start_node;
		break;
	case 3:
		AppendPoints(healed.pts, e2.pts, true);
		AppendPoints(healed.pts, e1.pts, false);
		healed.start_node = e2.end_node;
		healed.end_node = e1.end_node;
		break;
	default:
		AppendPoints(healed.pts, e2.pts, false);
		AppendPoints(healed.pts, e1.pts, false);
		healed.start_node = e2.start_node;
		healed.end_node = e1.end_node;
		break;
	}
	editor.LoadAtNodes(healed.start_node, healed.end_node);

	int32_t result;
	editor.Remove(edge2);
	if (mod_edge) {
		auto &edge = *editor.Find(edge1);
		edge.start_node = healed.start_node;
		edge.end_node = healed.end_node;
		edge.pts = std::move(healed.pts);
		result = common_node;
	} else {
		editor.Remove(edge1);
		healed.id = session.NextId("edge_data_edge_id_seq");
		result = healed.id;
		editor.Create(std::move(healed));
	}
	const auto &merged = *editor.Find(mod_edge ? edge1 : result);
	const auto start_node = merged.start_node;
	const auto end_node = merged.end_node;
	editor.Relink(start_node);
	if (end_node != start_node) {
		editor.Relink(end_node);
	}
	editor.Flush();
	session.Query("DELETE FROM " + session.Table("node") + " WHERE node_id = ?", {Value::INTEGER(common_node)});
	return result;
}

} // namespace topology

} // namespace duckdb
