#include "spatial/modules/topology/topology_ops.hpp"

#include "duckdb/common/algorithm.hpp"

#include <cmath>

namespace duckdb {

namespace topology {

//------------------------------------------------------------------------------
// ST_GetFaceGeometry
//------------------------------------------------------------------------------

GeosGeometry BuildFaceGeometry(TopoSession &session, const vector<const TopoEdge *> &edges) {
	const auto ctx = session.Geos();
	if (edges.empty()) {
		return GeosGeometry(ctx, GEOSGeom_createEmptyPolygon_r(ctx));
	}
	GeosCollection lines(ctx);
	lines.reserve(edges.size());
	for (auto edge : edges) {
		lines.add(session.MakeLine(edge->pts));
	}
	const auto collection = lines.get_collection();
	return collection.get_built_area();
}

Value GetFaceGeometry(TopoSession &session, int32_t face_id) {
	if (face_id == 0) {
		throw InvalidInputException("SQL/MM Spatial exception - universal face has no geometry");
	}
	TopoFace face;
	if (!session.LoadFace(face_id, face)) {
		throw InvalidInputException("SQL/MM Spatial exception - non-existent face.");
	}
	const auto edges =
	    session.LoadEdges("(left_face = ?) <> (right_face = ?)", {Value::INTEGER(face_id), Value::INTEGER(face_id)});
	vector<const TopoEdge *> boundary;
	for (auto &edge : edges) {
		boundary.push_back(&edge);
	}
	const auto polygon = BuildFaceGeometry(session, boundary);
	return session.ToValue(polygon.get_raw());
}

//------------------------------------------------------------------------------
// ST_GetFaceEdges
//------------------------------------------------------------------------------

namespace {

struct FaceCycle {
	vector<int32_t> edges;
	double area = 0;
	int32_t min_edge = 0;
};

} // namespace

vector<int32_t> GetFaceEdges(TopoSession &session, int32_t face_id) {
	EdgeMap edges;
	for (auto &edge :
	     session.LoadEdges("left_face = ? OR right_face = ?", {Value::INTEGER(face_id), Value::INTEGER(face_id)})) {
		const auto id = edge.id;
		edges[id] = std::move(edge);
	}
	auto get_edge = [&](int32_t id) -> const TopoEdge * {
		const auto entry = edges.find(id);
		return entry == edges.end() ? nullptr : &entry->second;
	};

	vector<int32_t> starts;
	for (auto &entry : edges) {
		auto &edge = entry.second;
		if (edge.left_face == edge.right_face) {
			continue;
		}
		starts.push_back(edge.left_face == face_id ? edge.id : -edge.id);
	}
	std::sort(starts.begin(), starts.end(), [](int32_t a, int32_t b) { return AbsValue(a) < AbsValue(b); });

	vector<FaceCycle> cycles;
	unordered_map<int32_t, bool> visited;
	for (auto start : starts) {
		if (visited[start]) {
			continue;
		}
		vector<int32_t> ring;
		if (!WalkRing(get_edge, start, 2 * edges.size() + 1, ring)) {
			throw InvalidInputException("Corrupted topology: ring of edge %d is not closed", start);
		}
		// Edges with the face on both sides are walked twice and do not bound it; what is left of the walk is cut
		// into simple cycles, one per ring of the face
		vector<int32_t> stack;
		for (auto signed_id : ring) {
			visited[signed_id] = true;
			auto &edge = edges[AbsValue(signed_id)];
			if (edge.left_face == edge.right_face) {
				continue;
			}
			stack.push_back(signed_id);
			const auto reached = signed_id > 0 ? edge.end_node : edge.start_node;
			for (idx_t i = stack.size(); i > 0; i--) {
				auto &candidate = edges[AbsValue(stack[i - 1])];
				const auto origin = stack[i - 1] > 0 ? candidate.start_node : candidate.end_node;
				if (origin != reached) {
					continue;
				}
				FaceCycle cycle;
				cycle.edges.assign(stack.begin() + NumericCast<int64_t>(i) - 1, stack.end());
				stack.resize(i - 1);
				cycles.push_back(std::move(cycle));
				break;
			}
		}
		if (!stack.empty()) {
			FaceCycle cycle;
			cycle.edges = std::move(stack);
			cycles.push_back(std::move(cycle));
		}
	}

	for (auto &cycle : cycles) {
		cycle.area = SignedArea(RingCoordinates(get_edge, cycle.edges));
		idx_t first = 0;
		for (idx_t i = 0; i < cycle.edges.size(); i++) {
			if (AbsValue(cycle.edges[i]) < AbsValue(cycle.edges[first])) {
				first = i;
			}
		}
		std::rotate(cycle.edges.begin(), cycle.edges.begin() + NumericCast<int64_t>(first), cycle.edges.end());
		cycle.min_edge = AbsValue(cycle.edges[0]);
	}
	// The shell comes first, then the holes
	std::sort(cycles.begin(), cycles.end(), [&](const FaceCycle &a, const FaceCycle &b) {
		const bool a_shell = face_id != 0 && a.area > 0;
		const bool b_shell = face_id != 0 && b.area > 0;
		if (a_shell != b_shell) {
			return a_shell;
		}
		return a.min_edge < b.min_edge;
	});

	vector<int32_t> result;
	for (auto &cycle : cycles) {
		result.insert(result.end(), cycle.edges.begin(), cycle.edges.end());
	}
	return result;
}

//------------------------------------------------------------------------------
// GetNodeByPoint / GetEdgeByPoint / GetFaceByPoint
//------------------------------------------------------------------------------

static void CheckTolerance(double tolerance) {
	if (!(tolerance >= 0)) {
		throw InvalidInputException("Tolerance must be a non-negative number");
	}
}

int32_t GetNodeByPoint(TopoSession &session, const Value &point, double tolerance) {
	CheckTolerance(tolerance);
	const auto p = ParsePoint(session, point);
	TopoBox box;
	box.Add(p);
	box.Expand(tolerance);
	int32_t found = 0;
	for (auto &node : session.LoadNodesInBox(box)) {
		if (std::hypot(node.pt.x - p.x, node.pt.y - p.y) > tolerance) {
			continue;
		}
		if (found != 0) {
			throw InvalidInputException("Two or more nodes found");
		}
		found = node.id;
	}
	return found;
}

int32_t GetEdgeByPoint(TopoSession &session, const Value &point, double tolerance) {
	CheckTolerance(tolerance);
	const auto p = ParsePoint(session, point);
	TopoBox box;
	box.Add(p);
	box.Expand(tolerance);
	int32_t found = 0;
	for (auto &edge : session.LoadEdgesInBox(box)) {
		if (PointLineDistance(edge.pts, p) > tolerance) {
			continue;
		}
		if (found != 0) {
			throw InvalidInputException("Two or more edges found");
		}
		found = edge.id;
	}
	return found;
}

int32_t GetFaceByPoint(TopoSession &session, const Value &point, double tolerance) {
	CheckTolerance(tolerance);
	const auto p = ParsePoint(session, point);
	TopoBox box;
	box.Add(p);
	box.Expand(tolerance);
	const auto edges = session.LoadEdgesInBox(box);

	// A point on an edge that has the same face on both sides is inside that face
	bool on_boundary = false;
	bool on_inner_edge = false;
	int32_t inner_face = 0;
	for (auto &edge : edges) {
		if (PointLineDistance(edge.pts, p) != 0) {
			continue;
		}
		if (edge.left_face == edge.right_face) {
			on_inner_edge = true;
			inner_face = edge.left_face;
		} else {
			on_boundary = true;
		}
	}
	int32_t found = 0;
	if (!on_boundary) {
		found = on_inner_edge ? inner_face : session.FaceContainingPoint(p);
		if (found > 0) {
			return found;
		}
	}
	for (auto &edge : edges) {
		if (edge.left_face == edge.right_face || PointLineDistance(edge.pts, p) > tolerance) {
			continue;
		}
		if (edge.left_face != 0 && edge.right_face != 0) {
			throw InvalidInputException("Two or more faces found");
		}
		const auto face = edge.left_face == 0 ? edge.right_face : edge.left_face;
		if (found != 0 && found != face) {
			throw InvalidInputException("Two or more faces found");
		}
		found = face;
	}
	return found;
}

//------------------------------------------------------------------------------
// GetNodeEdges / GetRingEdges
//------------------------------------------------------------------------------

vector<int32_t> GetNodeEdges(TopoSession &session, int32_t node_id) {
	struct Incident {
		int32_t edge;
		double azimuth;
	};
	vector<Incident> incident;
	const auto two_pi = 2 * std::acos(-1.0);
	auto azimuth = [&](const TopoEdge &edge, bool outgoing) {
		// EdgeEndAngle is counter-clockwise from east, azimuths are clockwise from north
		auto value = std::acos(-1.0) / 2 - EdgeEndAngle(edge, outgoing);
		if (value < 0) {
			value += two_pi;
		}
		return value;
	};
	for (auto &edge :
	     session.LoadEdges("start_node = ? OR end_node = ?", {Value::INTEGER(node_id), Value::INTEGER(node_id)})) {
		if (edge.start_node == node_id) {
			incident.push_back(Incident {edge.id, azimuth(edge, true)});
		}
		if (edge.end_node == node_id) {
			incident.push_back(Incident {-edge.id, azimuth(edge, false)});
		}
	}
	std::sort(incident.begin(), incident.end(), [](const Incident &a, const Incident &b) {
		if (a.azimuth != b.azimuth) {
			return a.azimuth < b.azimuth;
		}
		return a.edge < b.edge;
	});
	vector<int32_t> result;
	for (auto &entry : incident) {
		result.push_back(entry.edge);
	}
	return result;
}

vector<int32_t> GetRingEdges(TopoSession &session, int32_t edge_id, const Value &max_edges) {
	vector<int32_t> ring;
	if (edge_id == 0) {
		return ring;
	}
	TopoEdge first;
	if (!session.LoadEdge(AbsValue(edge_id), first)) {
		return ring;
	}
	// The whole ring has the same face on its left, so one query brings every edge the walk can visit
	const auto face = edge_id > 0 ? first.left_face : first.right_face;
	EdgeMap edges;
	for (auto &edge :
	     session.LoadEdges("left_face = ? OR right_face = ?", {Value::INTEGER(face), Value::INTEGER(face)})) {
		const auto id = edge.id;
		edges[id] = std::move(edge);
	}
	auto get_edge = [&](int32_t id) -> const TopoEdge * {
		auto entry = edges.find(id);
		if (entry == edges.end()) {
			TopoEdge edge;
			if (!session.LoadEdge(id, edge)) {
				return nullptr;
			}
			entry = edges.emplace(id, std::move(edge)).first;
		}
		return &entry->second;
	};
	idx_t limit = 100000000;
	if (!max_edges.IsNull()) {
		const auto requested = max_edges.GetValue<int32_t>();
		if (requested <= 0) {
			throw InvalidInputException("Max traversing limit must be positive");
		}
		limit = NumericCast<idx_t>(requested);
	}
	if (!WalkRing(get_edge, edge_id, limit, ring)) {
		if (ring.size() >= limit && !max_edges.IsNull()) {
			throw InvalidInputException("Max traversing limit hit: %d", max_edges.GetValue<int32_t>());
		}
		throw InvalidInputException("Corrupted topology: ring of edge %d is not closed", edge_id);
	}
	return ring;
}

} // namespace topology

} // namespace duckdb
