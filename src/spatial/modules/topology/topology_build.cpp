#include "spatial/modules/topology/topology_ops.hpp"

#include "duckdb/common/algorithm.hpp"
#include "duckdb/common/map.hpp"
#include "duckdb/common/unordered_set.hpp"

namespace duckdb {

namespace topology {

namespace {

//! Finds the smallest shell ring holding a point, which is the face the point lies in
class ShellLocator {
public:
	ShellLocator(GEOSContextHandle_t ctx, const GraphAnalysis &analysis_p) : analysis(analysis_p), index(ctx) {
		for (idx_t r = 0; r < analysis.rings.size(); r++) {
			auto &ring = analysis.rings[r];
			if (!ring.outer && ring.closed) {
				index.Insert(ring.box, r);
			}
		}
	}

	//! Returns the ring index, or INVALID_INDEX when the point is in no shell. Shells of the ignored component are
	//! skipped: they can touch the point but never contain it
	idx_t Locate(const TopoPoint &p, idx_t ignored_component) {
		TopoBox box;
		box.Add(p);
		index.Query(box, candidates);
		idx_t best = DConstants::INVALID_INDEX;
		for (auto r : candidates) {
			auto &ring = analysis.rings[r];
			if (ring.component == ignored_component) {
				continue;
			}
			if (best != DConstants::INVALID_INDEX && analysis.rings[best].area <= ring.area) {
				continue;
			}
			if (PointInRing(ring.coords, p)) {
				best = r;
			}
		}
		return best;
	}

private:
	const GraphAnalysis &analysis;
	BoxIndex index;
	vector<idx_t> candidates;
};

struct PointKey {
	double x;
	double y;
	bool operator<(const PointKey &other) const {
		return x != other.x ? x < other.x : y < other.y;
	}
};

PointKey KeyOf(const TopoPoint &p) {
	return PointKey {p.x, p.y};
}

struct SplitPosition {
	idx_t segment;
	bool at_vertex;
	double offset;
	TopoPoint pt;
};

} // namespace

//------------------------------------------------------------------------------
// ST_CreateTopoGeo
//------------------------------------------------------------------------------

static void CollectComponents(TopoSession &session, const GEOSGeometry *geom, vector<TopoPoint> &points,
                              vector<vector<TopoPoint>> &lines) {
	const auto ctx = session.Geos();
	if (GEOSisEmpty_r(ctx, geom)) {
		return;
	}
	switch (GEOSGeomTypeId_r(ctx, geom)) {
	case GEOS_POINT:
		points.push_back(session.Coordinates(geom)[0]);
		break;
	case GEOS_LINESTRING:
	case GEOS_LINEARRING: {
		auto pts = session.Coordinates(geom);
		// The end points of the input lines become nodes, as in ISO
		if (pts.front() != pts.back()) {
			points.push_back(pts.front());
			points.push_back(pts.back());
		}
		lines.push_back(std::move(pts));
		break;
	}
	case GEOS_POLYGON: {
		lines.push_back(session.Coordinates(GEOSGetExteriorRing_r(ctx, geom)));
		const auto holes = GEOSGetNumInteriorRings_r(ctx, geom);
		for (int i = 0; i < holes; i++) {
			lines.push_back(session.Coordinates(GEOSGetInteriorRingN_r(ctx, geom, i)));
		}
		break;
	}
	case GEOS_MULTIPOINT:
	case GEOS_MULTILINESTRING:
	case GEOS_MULTIPOLYGON:
	case GEOS_GEOMETRYCOLLECTION: {
		const auto parts = GEOSGetNumGeometries_r(ctx, geom);
		for (int i = 0; i < parts; i++) {
			CollectComponents(session, GEOSGetGeometryN_r(ctx, geom, i), points, lines);
		}
		break;
	}
	default:
		throw InvalidInputException("ST_CreateTopoGeo does not support this geometry type");
	}
}

static void CollectLines(TopoSession &session, const GEOSGeometry *geom, vector<vector<TopoPoint>> &lines) {
	const auto ctx = session.Geos();
	if (GEOSisEmpty_r(ctx, geom)) {
		return;
	}
	const auto type = GEOSGeomTypeId_r(ctx, geom);
	if (type == GEOS_LINESTRING || type == GEOS_LINEARRING) {
		lines.push_back(session.Coordinates(geom));
		return;
	}
	const auto parts = GEOSGetNumGeometries_r(ctx, geom);
	for (int i = 0; i < parts; i++) {
		CollectLines(session, GEOSGetGeometryN_r(ctx, geom, i), lines);
	}
}

//! Node the linework, merge it into maximal edges and cut these at the requested points
static vector<vector<TopoPoint>> NodeLinework(TopoSession &session, const vector<vector<TopoPoint>> &input,
                                              const vector<TopoPoint> &split_points) {
	vector<vector<TopoPoint>> merged;
	if (input.empty()) {
		return merged;
	}
	{
		GeosCollection collection(session.Geos());
		collection.reserve(input.size());
		for (auto &line : input) {
			collection.add(session.MakeLine(line));
		}
		const auto linework = collection.get_collection();
		const auto noded = linework.get_unary_union();
		const auto lines = noded.get_linemerged(false);
		CollectLines(session, lines.get_raw(), merged);
	}
	if (split_points.empty()) {
		return merged;
	}

	BoxIndex index(session.Geos());
	for (idx_t i = 0; i < merged.size(); i++) {
		TopoBox box;
		box.Add(merged[i]);
		index.Insert(box, i);
	}
	vector<vector<SplitPosition>> splits(merged.size());
	vector<idx_t> candidates;
	for (auto &p : split_points) {
		TopoBox box;
		box.Add(p);
		index.Query(box, candidates);
		for (auto i : candidates) {
			auto &line = merged[i];
			SplitPosition split;
			if (!LocatePointOnLine(line, p, split.segment, split.at_vertex)) {
				continue;
			}
			const bool closed = line.front() == line.back();
			if (split.at_vertex && !closed && (split.segment == 0 || split.segment == line.size() - 1)) {
				continue;
			}
			if (split.at_vertex && split.segment == line.size() - 1) {
				split.segment = 0;
			}
			auto &origin = line[split.segment];
			split.offset = split.at_vertex ? 0 : std::hypot(p.x - origin.x, p.y - origin.y);
			split.pt = split.at_vertex ? origin : p;
			splits[i].push_back(split);
		}
	}

	vector<vector<TopoPoint>> result;
	for (idx_t i = 0; i < merged.size(); i++) {
		auto &line = merged[i];
		auto &cuts = splits[i];
		if (cuts.empty()) {
			result.push_back(std::move(line));
			continue;
		}
		std::sort(cuts.begin(), cuts.end(), [](const SplitPosition &a, const SplitPosition &b) {
			return a.segment != b.segment ? a.segment < b.segment : a.offset < b.offset;
		});
		// Walk the line once, starting a new piece at every cut
		vector<vector<TopoPoint>> pieces;
		vector<TopoPoint> current;
		idx_t next_cut = 0;
		for (idx_t v = 0; v < line.size(); v++) {
			if (current.empty() || current.back() != line[v]) {
				current.push_back(line[v]);
			}
			while (next_cut < cuts.size() && cuts[next_cut].segment == v) {
				auto &cut = cuts[next_cut++];
				if (current.back() != cut.pt) {
					current.push_back(cut.pt);
				}
				if (current.size() > 1) {
					pieces.push_back(current);
					current.clear();
					current.push_back(cut.pt);
				}
			}
		}
		if (current.size() > 1) {
			pieces.push_back(std::move(current));
		}
		// A ring has no end points of its own: its arbitrary first vertex must not become a node
		const bool closed = line.front() == line.back();
		const bool cut_at_start = cuts[0].segment == 0 && cuts[0].at_vertex;
		if (closed && !cut_at_start && pieces.size() > 1) {
			auto &last = pieces.back();
			auto &first = pieces.front();
			last.insert(last.end(), first.begin() + 1, first.end());
			pieces.erase(pieces.begin());
		}
		for (auto &piece : pieces) {
			result.push_back(std::move(piece));
		}
	}
	return result;
}

void CreateTopoGeo(TopoSession &session, const Value &collection) {
	auto counts = session.Query("SELECT (SELECT count(*) FROM " + session.Table("node") + "), (SELECT count(*) FROM " +
	                            session.Table("edge_data") + "), (SELECT count(*) FROM " + session.Table("face") + ")");
	if (counts->GetValue(0, 0).GetValue<int64_t>() > 0 || counts->GetValue(1, 0).GetValue<int64_t>() > 0) {
		throw InvalidInputException("SQL/MM Spatial exception - non-empty view");
	}
	if (counts->GetValue(2, 0).GetValue<int64_t>() != 1) {
		throw InvalidInputException("SQL/MM Spatial exception - non-empty face view");
	}

	vector<TopoPoint> points;
	vector<vector<TopoPoint>> input_lines;
	{
		const auto geom = session.ToGeos(collection);
		CollectComponents(session, geom.get_raw(), points, input_lines);
	}
	auto lines = NodeLinework(session, input_lines, points);
	std::sort(lines.begin(), lines.end(), [](const vector<TopoPoint> &a, const vector<TopoPoint> &b) {
		return std::lexicographical_compare(
		    a.begin(), a.end(), b.begin(), b.end(),
		    [](const TopoPoint &p, const TopoPoint &q) { return p.x != q.x ? p.x < q.x : p.y < q.y; });
	});

	// Nodes: edge end points first, then the input points that are not on any edge
	map<PointKey, idx_t> node_index;
	vector<TopoNode> nodes;
	auto node_at = [&](const TopoPoint &p, bool isolated) -> idx_t {
		const auto entry = node_index.find(KeyOf(p));
		if (entry != node_index.end()) {
			return entry->second;
		}
		TopoNode node;
		node.pt = p;
		node.has_face = isolated;
		nodes.push_back(node);
		node_index[KeyOf(p)] = nodes.size() - 1;
		return nodes.size() - 1;
	};
	vector<TopoEdge> edges(lines.size());
	const auto edge_ids = session.NextIds("edge_data_edge_id_seq", lines.size());
	vector<idx_t> start_index(lines.size());
	vector<idx_t> end_index(lines.size());
	for (idx_t i = 0; i < lines.size(); i++) {
		start_index[i] = node_at(lines[i].front(), false);
		end_index[i] = node_at(lines[i].back(), false);
	}
	std::sort(points.begin(), points.end(),
	          [](const TopoPoint &p, const TopoPoint &q) { return p.x != q.x ? p.x < q.x : p.y < q.y; });
	for (auto &p : points) {
		node_at(p, true);
	}
	const auto node_ids = session.NextIds("node_node_id_seq", nodes.size());
	for (idx_t i = 0; i < nodes.size(); i++) {
		nodes[i].id = node_ids[i];
	}
	for (idx_t i = 0; i < lines.size(); i++) {
		auto &edge = edges[i];
		edge.id = edge_ids[i];
		edge.start_node = nodes[start_index[i]].id;
		edge.end_node = nodes[end_index[i]].id;
		edge.pts = std::move(lines[i]);
	}

	GraphAnalysis analysis;
	AnalyzeGraph(edges, analysis);

	// Every ring that is not the outer boundary of its connected component is the shell of a new face
	idx_t shell_count = 0;
	for (auto &ring : analysis.rings) {
		shell_count += ring.outer ? 0 : 1;
	}
	const auto face_ids = session.NextIds("face_face_id_seq", shell_count);
	vector<int32_t> ring_face(analysis.rings.size(), 0);
	vector<Value> face_id_values;
	vector<Value> face_mbr_values;
	idx_t next_face = 0;
	for (idx_t r = 0; r < analysis.rings.size(); r++) {
		auto &ring = analysis.rings[r];
		if (ring.outer) {
			continue;
		}
		ring_face[r] = face_ids[next_face++];
		face_id_values.push_back(Value::INTEGER(ring_face[r]));
		face_mbr_values.push_back(session.BoxValue(ring.box));
	}
	// Outer boundaries lie in the face of the smallest shell of another component around them
	ShellLocator locator(session.Geos(), analysis);
	for (idx_t r = 0; r < analysis.rings.size(); r++) {
		auto &ring = analysis.rings[r];
		if (!ring.outer || ring.coords.empty()) {
			continue;
		}
		const auto shell = locator.Locate(ring.coords[0], ring.component);
		ring_face[r] = shell == DConstants::INVALID_INDEX ? 0 : ring_face[shell];
	}
	for (auto &node : nodes) {
		if (!node.has_face) {
			continue;
		}
		const auto shell = locator.Locate(node.pt, DConstants::INVALID_INDEX);
		node.face = shell == DConstants::INVALID_INDEX ? 0 : ring_face[shell];
	}

	if (!face_id_values.empty()) {
		session.Query("INSERT INTO " + session.Table("face") +
		                  "(face_id, mbr) SELECT unnest(?::INTEGER[]), unnest(?::GEOMETRY[])",
		              {Value::LIST(LogicalType::INTEGER, std::move(face_id_values)),
		               Value::LIST(LogicalType::GEOMETRY(), std::move(face_mbr_values))});
	}
	if (!nodes.empty()) {
		vector<Value> ids;
		vector<Value> faces;
		vector<Value> geoms;
		for (auto &node : nodes) {
			ids.push_back(Value::INTEGER(node.id));
			faces.push_back(node.has_face ? Value::INTEGER(node.face) : Value(LogicalType::INTEGER));
			geoms.push_back(session.PointValue(node.pt));
		}
		session.Query("INSERT INTO " + session.Table("node") +
		                  "(node_id, containing_face, geom) SELECT unnest(?::INTEGER[]), unnest(?::INTEGER[]), "
		                  "unnest(?::GEOMETRY[])",
		              {Value::LIST(LogicalType::INTEGER, std::move(ids)),
		               Value::LIST(LogicalType::INTEGER, std::move(faces)),
		               Value::LIST(LogicalType::GEOMETRY(), std::move(geoms))});
	}
	if (!edges.empty()) {
		vector<vector<Value>> columns(7);
		vector<Value> geoms;
		for (idx_t i = 0; i < edges.size(); i++) {
			auto &edge = edges[i];
			columns[0].push_back(Value::INTEGER(edge.id));
			columns[1].push_back(Value::INTEGER(edge.start_node));
			columns[2].push_back(Value::INTEGER(edge.end_node));
			columns[3].push_back(Value::INTEGER(analysis.next_left[i]));
			columns[4].push_back(Value::INTEGER(analysis.next_right[i]));
			columns[5].push_back(Value::INTEGER(ring_face[analysis.left_ring[i]]));
			columns[6].push_back(Value::INTEGER(ring_face[analysis.right_ring[i]]));
			geoms.push_back(session.LineValue(edge.pts));
		}
		vector<Value> params;
		for (auto &column : columns) {
			params.push_back(Value::LIST(LogicalType::INTEGER, std::move(column)));
		}
		params.push_back(Value::LIST(LogicalType::GEOMETRY(), std::move(geoms)));
		session.Query(
		    "INSERT INTO " + session.Table("edge_data") +
		        "(edge_id, start_node, end_node, next_left_edge, abs_next_left_edge, next_right_edge, "
		        "abs_next_right_edge, left_face, right_face, geom) "
		        "SELECT id, s, e, nl, abs(nl), nr, abs(nr), lf, rf, g FROM (SELECT unnest(?::INTEGER[]) AS id, "
		        "unnest(?::INTEGER[]) AS s, unnest(?::INTEGER[]) AS e, unnest(?::INTEGER[]) AS nl, "
		        "unnest(?::INTEGER[]) AS nr, unnest(?::INTEGER[]) AS lf, unnest(?::INTEGER[]) AS rf, "
		        "unnest(?::GEOMETRY[]) AS g)",
		    std::move(params));
	}
}

//------------------------------------------------------------------------------
// ValidateTopology
//------------------------------------------------------------------------------

void ValidateTopology(TopoSession &session, TopoRows &rows) {
	const auto ctx = session.Geos();
	auto nodes = session.LoadNodes("true ORDER BY node_id");
	auto edges = session.LoadEdges("true ORDER BY edge_id");
	auto faces = session.LoadFaces("true ORDER BY face_id");

	auto report = [&](const char *error, int32_t id1) {
		rows.push_back({Value(error), Value::INTEGER(id1), Value(LogicalType::INTEGER)});
	};
	auto report_pair = [&](const char *error, int32_t id1, int32_t id2) {
		rows.push_back({Value(error), Value::INTEGER(id1), Value::INTEGER(id2)});
	};

	unordered_map<int32_t, idx_t> node_index;
	for (idx_t i = 0; i < nodes.size(); i++) {
		node_index[nodes[i].id] = i;
	}

	// Coincident nodes
	{
		map<PointKey, vector<int32_t>> by_location;
		for (auto &node : nodes) {
			by_location[KeyOf(node.pt)].push_back(node.id);
		}
		vector<pair<int32_t, int32_t>> pairs;
		for (auto &entry : by_location) {
			auto &ids = entry.second;
			for (idx_t i = 0; i < ids.size(); i++) {
				for (idx_t j = i + 1; j < ids.size(); j++) {
					pairs.emplace_back(ids[i], ids[j]);
				}
			}
		}
		std::sort(pairs.begin(), pairs.end());
		for (auto &entry : pairs) {
			report_pair("coincident nodes", entry.first, entry.second);
		}
	}

	BoxIndex edge_boxes(ctx);
	vector<TopoBox> boxes(edges.size());
	for (idx_t i = 0; i < edges.size(); i++) {
		boxes[i].Add(edges[i].pts);
		if (!boxes[i].IsEmpty()) {
			edge_boxes.Insert(boxes[i], i);
		}
	}
	vector<idx_t> candidates;

	// Edge crosses node
	{
		vector<pair<int32_t, int32_t>> pairs;
		for (auto &node : nodes) {
			TopoBox box;
			box.Add(node.pt);
			edge_boxes.Query(box, candidates);
			for (auto i : candidates) {
				auto &edge = edges[i];
				if (edge.start_node == node.id || edge.end_node == node.id) {
					continue;
				}
				idx_t segment;
				bool at_vertex;
				if (LocatePointOnLine(edge.pts, node.pt, segment, at_vertex)) {
					pairs.emplace_back(edge.id, node.id);
				}
			}
		}
		std::sort(pairs.begin(), pairs.end());
		for (auto &entry : pairs) {
			report_pair("edge crosses node", entry.first, entry.second);
		}
	}

	// Invalid and non simple edges
	vector<bool> usable(edges.size(), true);
	vector<GeosGeometry> edge_lines;
	edge_lines.reserve(edges.size());
	for (idx_t i = 0; i < edges.size(); i++) {
		auto &edge = edges[i];
		bool distinct = false;
		for (idx_t v = 1; v < edge.pts.size(); v++) {
			distinct = distinct || edge.pts[v] != edge.pts[0];
		}
		if (!distinct) {
			usable[i] = false;
			edge_lines.emplace_back(ctx, nullptr);
			report("invalid edge", edge.id);
			continue;
		}
		edge_lines.push_back(session.MakeLine(edge.pts));
		if (!edge_lines.back().is_simple()) {
			report("edge not simple", edge.id);
		}
	}

	// Edge crosses edge
	{
		vector<pair<int32_t, int32_t>> pairs;
		for (idx_t i = 0; i < edges.size(); i++) {
			if (!usable[i]) {
				continue;
			}
			edge_boxes.Query(boxes[i], candidates);
			for (auto j : candidates) {
				if (j <= i || !usable[j]) {
					continue;
				}
				const auto matrix = GEOSRelateBoundaryNodeRule_r(ctx, edge_lines[i].get_raw(), edge_lines[j].get_raw(),
				                                                 GEOSRELATE_BNR_ENDPOINT);
				if (!matrix) {
					continue;
				}
				const bool disjoint = GEOSRelatePatternMatch_r(ctx, matrix, "FF*F*****") == 1;
				GEOSFree_r(ctx, matrix);
				if (!disjoint) {
					pairs.emplace_back(edges[i].id, edges[j].id);
				}
			}
		}
		std::sort(pairs.begin(), pairs.end());
		for (auto &entry : pairs) {
			report_pair("edge crosses edge", entry.first, entry.second);
		}
	}

	// Edge end points against their nodes
	for (idx_t pass = 0; pass < 2; pass++) {
		for (auto &edge : edges) {
			if (edge.pts.empty()) {
				continue;
			}
			const auto node_id = pass == 0 ? edge.start_node : edge.end_node;
			const auto entry = node_index.find(node_id);
			const auto &pt = pass == 0 ? edge.pts.front() : edge.pts.back();
			if (entry == node_index.end() || nodes[entry->second].pt != pt) {
				report_pair(pass == 0 ? "edge start node geometry mismatch" : "edge end node geometry mismatch",
				            edge.id, node_id);
			}
		}
	}
	const bool geometry_valid = rows.empty();

	// Faces without edges
	unordered_set<int32_t> used_faces;
	for (auto &edge : edges) {
		used_faces.insert(edge.left_face);
		used_faces.insert(edge.right_face);
	}
	unordered_set<int32_t> empty_faces;
	for (auto &face : faces) {
		if (face.id != 0 && used_faces.find(face.id) == used_faces.end()) {
			empty_faces.insert(face.id);
			report("face without edges", face.id);
		}
	}

	// Edge linking, compared with what the geometry implies
	vector<TopoEdge> valid_edges;
	for (idx_t i = 0; i < edges.size(); i++) {
		if (usable[i]) {
			valid_edges.push_back(edges[i]);
		}
	}
	GraphAnalysis analysis;
	AnalyzeGraph(valid_edges, analysis);
	for (idx_t i = 0; i < valid_edges.size(); i++) {
		if (valid_edges[i].next_right != analysis.next_right[i]) {
			report_pair("invalid next_right_edge", valid_edges[i].id, analysis.next_right[i]);
		}
		if (valid_edges[i].next_left != analysis.next_left[i]) {
			report_pair("invalid next_left_edge", valid_edges[i].id, analysis.next_left[i]);
		}
	}

	// The remaining checks walk the rings, which only makes sense on sound linework
	if (!geometry_valid) {
		return;
	}

	const auto invalid = DConstants::INVALID_INDEX;
	vector<int32_t> ring_face(analysis.rings.size(), 0);
	vector<bool> ring_labelled(analysis.rings.size(), true);
	unordered_map<int32_t, idx_t> face_shell;
	vector<unordered_set<int32_t>> ring_labels(analysis.rings.size());
	for (idx_t i = 0; i < valid_edges.size(); i++) {
		ring_labels[analysis.left_ring[i]].insert(valid_edges[i].left_face);
		ring_labels[analysis.right_ring[i]].insert(valid_edges[i].right_face);
	}
	for (idx_t r = 0; r < analysis.rings.size(); r++) {
		auto &ring = analysis.rings[r];
		const auto ring_id = ring.edges[0];
		if (ring_labels[r].size() != 1) {
			ring_labelled[r] = false;
			report("mixed face labeling in ring", ring_id);
			continue;
		}
		ring_labelled[r] = true;
		ring_face[r] = *ring_labels[r].begin();
		if (ring.outer) {
			continue;
		}
		if (ring_face[r] == 0) {
			report_pair("universal face has shell rings", 0, ring_id);
			continue;
		}
		const auto entry = face_shell.find(ring_face[r]);
		if (entry != face_shell.end()) {
			report_pair("face has multiple shells", ring_face[r], ring_id);
			continue;
		}
		face_shell[ring_face[r]] = r;
	}

	for (auto &face : faces) {
		if (face.id == 0 || empty_faces.find(face.id) != empty_faces.end()) {
			continue;
		}
		const auto entry = face_shell.find(face.id);
		if (entry == face_shell.end()) {
			report("face has no rings", face.id);
			continue;
		}
		if (!face.has_mbr || !(face.mbr == analysis.rings[entry->second].box)) {
			report("face has wrong mbr", face.id);
		}
	}

	ShellLocator locator(ctx, analysis);
	auto face_at = [&](const TopoPoint &p, idx_t ignored_component) {
		const auto shell = locator.Locate(p, ignored_component);
		return shell == invalid || !ring_labelled[shell] ? 0 : ring_face[shell];
	};
	for (idx_t r = 0; r < analysis.rings.size(); r++) {
		auto &ring = analysis.rings[r];
		if (!ring.outer || !ring_labelled[r] || ring.coords.empty()) {
			continue;
		}
		if (face_at(ring.coords[0], ring.component) != ring_face[r]) {
			report("hole not in advertised face", ring.edges[0]);
		}
	}

	unordered_set<int32_t> connected_nodes;
	for (auto &edge : edges) {
		connected_nodes.insert(edge.start_node);
		connected_nodes.insert(edge.end_node);
	}
	for (auto &node : nodes) {
		const bool connected = connected_nodes.find(node.id) != connected_nodes.end();
		if (connected) {
			if (node.has_face) {
				report("not-isolated node has not-null containing_face", node.id);
			}
			continue;
		}
		if (!node.has_face) {
			report("isolated node has null containing_face", node.id);
			continue;
		}
		if (face_at(node.pt, invalid) != node.face) {
			report("isolated node has wrong containing_face", node.id);
		}
	}

	// Faces must not share interior points
	map<int32_t, vector<const TopoEdge *>> face_edges;
	for (auto &edge : valid_edges) {
		if (edge.left_face == edge.right_face) {
			continue;
		}
		if (edge.left_face != 0) {
			face_edges[edge.left_face].push_back(&edge);
		}
		if (edge.right_face != 0) {
			face_edges[edge.right_face].push_back(&edge);
		}
	}
	vector<int32_t> polygon_faces;
	vector<GeosGeometry> polygons;
	BoxIndex polygon_boxes(ctx);
	vector<TopoBox> polygon_extents;
	for (auto &entry : face_edges) {
		auto polygon = BuildFaceGeometry(session, entry.second);
		if (polygon.is_empty()) {
			continue;
		}
		TopoBox box;
		polygon.get_extent(box.xmin, box.ymin, box.xmax, box.ymax);
		polygon_boxes.Insert(box, polygons.size());
		polygon_extents.push_back(box);
		polygon_faces.push_back(entry.first);
		polygons.push_back(std::move(polygon));
	}
	for (idx_t i = 0; i < polygons.size(); i++) {
		polygon_boxes.Query(polygon_extents[i], candidates);
		std::sort(candidates.begin(), candidates.end());
		for (auto j : candidates) {
			if (j <= i) {
				continue;
			}
			if (polygons[i].relate_pattern(polygons[j], "T*F**F***")) {
				report_pair("face within face", polygon_faces[i], polygon_faces[j]);
			} else if (polygons[j].relate_pattern(polygons[i], "T*F**F***")) {
				report_pair("face within face", polygon_faces[j], polygon_faces[i]);
			} else if (polygons[i].relate_pattern(polygons[j], "T*T***T**")) {
				report_pair("face overlaps face", polygon_faces[i], polygon_faces[j]);
			}
		}
	}
}

} // namespace topology

} // namespace duckdb
