#include "spatial/modules/topology/topology_session.hpp"

#include "duckdb/catalog/catalog.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/main/client_context.hpp"
#include "duckdb/main/database_manager.hpp"
#include "duckdb/parser/keyword_helper.hpp"
#include "duckdb/storage/buffer_manager.hpp"
#include "duckdb/storage/object_cache.hpp"
#include "duckdb/transaction/duck_transaction.hpp"

#include "spatial/modules/geos/geos_serde.hpp"

namespace duckdb {

namespace topology {

//------------------------------------------------------------------------------
// Write lock
//------------------------------------------------------------------------------

//! Serializes topology edits issued through different connections of one database
class TopologyLock : public ObjectCacheEntry {
public:
	static string ObjectType() {
		return "spatial_topology_lock";
	}
	string GetObjectType() override {
		return ObjectType();
	}
	optional_idx GetEstimatedCacheMemory() const override {
		return optional_idx();
	}

	mutex lock;
};

//------------------------------------------------------------------------------
// Session
//------------------------------------------------------------------------------

static const char *const EDGE_COLUMNS =
    "edge_id, start_node, end_node, next_left_edge, next_right_edge, left_face, right_face, geom";

TopoSession::TopoSession(ClientContext &context_p, bool write)
    : context(context_p), con(*context_p.db), arena(BufferAllocator::Get(context_p)) {
	catalog = DatabaseManager::GetDefaultDatabase(context);
	ctx = GEOS_init_r();
	GEOSContext_setErrorMessageHandler_r(
	    ctx, [](const char *message, void *) { throw InvalidInputException(message); }, nullptr);
	try {
		if (write) {
			CheckCallerTransaction();
			lock_entry = ObjectCache::GetObjectCache(context).GetOrCreate<TopologyLock>(TopologyLock::ObjectType());
			if (lock_entry) {
				write_lock = unique_lock<mutex>(lock_entry->lock);
			}
		}
		con.BeginTransaction();
		active = true;
	} catch (...) {
		GEOS_finish_r(ctx);
		throw;
	}
}

TopoSession::~TopoSession() {
	statements.clear();
	if (active) {
		try {
			con.Rollback();
		} catch (...) { // NOLINT
		}
	}
	GEOS_finish_r(ctx);
}

void TopoSession::CheckCallerTransaction() const {
	if (context.transaction.IsAutoCommit()) {
		return;
	}
	auto &transaction = Transaction::Get(context, Catalog::GetCatalog(context, catalog));
	if (transaction.IsDuckTransaction() && transaction.Cast<DuckTransaction>().ChangesMade()) {
		throw TransactionException(
		    "Topology editing functions run in their own transaction and cannot see the uncommitted changes of the "
		    "current transaction: COMMIT or ROLLBACK it before editing a topology");
	}
}

void TopoSession::Commit() {
	statements.clear();
	active = false;
	try {
		con.Commit();
	} catch (std::exception &ex) {
		Rethrow(ErrorData(ex));
	}
}

void TopoSession::Rethrow(const ErrorData &error) const {
	if (error.Type() == ExceptionType::TRANSACTION) {
		throw TransactionException(
		    "Topology tables are being modified by another transaction (%s). Topology functions run in their own "
		    "transaction: commit or roll back any open transaction that changed the topology tables and retry",
		    error.RawMessage());
	}
	error.Throw();
}

unique_ptr<MaterializedQueryResult> TopoSession::Query(const string &sql, vector<Value> params) {
	auto entry = statements.find(sql);
	if (entry == statements.end()) {
		auto prepared = con.Prepare(sql);
		if (prepared->HasError()) {
			Rethrow(prepared->GetErrorObject());
		}
		entry = statements.emplace(sql, std::move(prepared)).first;
	}
	auto result = entry->second->Execute(params, false);
	if (result->HasError()) {
		Rethrow(result->GetErrorObject());
	}
	return unique_ptr_cast<QueryResult, MaterializedQueryResult>(std::move(result));
}

void TopoSession::Execute(const string &sql) {
	auto result = con.Query(sql);
	if (result->HasError()) {
		Rethrow(result->GetErrorObject());
	}
}

//------------------------------------------------------------------------------
// Registry
//------------------------------------------------------------------------------

static string Quote(const string &identifier) {
	return KeywordHelper::WriteQuoted(identifier, '"');
}

string TopoSession::Registry() const {
	return Quote(catalog) + ".topology.topology";
}

string TopoSession::SchemaFor(const string &catalog, const string &name) {
	return Quote(catalog) + "." + Quote(name);
}

string TopoSession::Table(const char *table) const {
	return schema + "." + table;
}

void TopoSession::CheckName(const string &name) {
	bool valid = !name.empty() && name.size() <= 63;
	for (idx_t i = 0; valid && i < name.size(); i++) {
		const auto c = name[i];
		const bool alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
		const bool digit = c >= '0' && c <= '9';
		valid = alpha || (digit && i > 0);
	}
	if (!valid) {
		throw InvalidInputException("SQL/MM Spatial exception - invalid topology name");
	}
}

bool TopoSession::RegistryExists() {
	auto result = Query("SELECT count(*) FROM duckdb_tables() WHERE database_name = ? AND schema_name = 'topology' "
	                    "AND table_name = 'topology'",
	                    {Value(catalog)});
	return result->GetValue(0, 0).GetValue<int64_t>() > 0;
}

void TopoSession::CreateRegistry() {
	const auto prefix = Quote(catalog) + ".topology";
	Execute("CREATE SCHEMA IF NOT EXISTS " + prefix);
	Execute("CREATE SEQUENCE IF NOT EXISTS " + prefix + ".topology_id_seq");
	Execute("CREATE TABLE IF NOT EXISTS " + prefix +
	        ".topology(id INTEGER PRIMARY KEY, name VARCHAR NOT NULL UNIQUE, srid INTEGER NOT NULL, "
	        "\"precision\" DOUBLE NOT NULL, hasz BOOLEAN NOT NULL DEFAULT false)");
}

bool TopoSession::TryOpenTopology(const string &name) {
	CheckName(name);
	if (!RegistryExists()) {
		return false;
	}
	auto result = Query("SELECT id, name, srid, \"precision\" FROM " + Registry() + " WHERE lower(name) = lower(?)",
	                    {Value(name)});
	if (result->RowCount() == 0) {
		return false;
	}
	info.id = result->GetValue(0, 0).GetValue<int32_t>();
	info.name = result->GetValue(1, 0).GetValue<string>();
	info.srid = result->GetValue(2, 0).GetValue<int32_t>();
	info.precision = result->GetValue(3, 0).GetValue<double>();
	schema = SchemaFor(catalog, info.name);
	return true;
}

void TopoSession::OpenTopology(const Value &name) {
	if (name.IsNull()) {
		throw InvalidInputException("SQL/MM Spatial exception - null argument");
	}
	if (!TryOpenTopology(name.GetValue<string>())) {
		throw InvalidInputException("SQL/MM Spatial exception - invalid topology name");
	}
}

void TopoSession::CheckSRID(const string &crs) const {
	if (crs.empty()) {
		return;
	}
	const auto expected = "EPSG:" + to_string(info.srid);
	if (info.srid <= 0 || !StringUtil::CIEquals(crs, expected)) {
		throw InvalidInputException("Geometry SRID (%s) does not match topology SRID (%d)", crs, info.srid);
	}
}

//------------------------------------------------------------------------------
// Geometry conversion
//------------------------------------------------------------------------------

GeosGeometry TopoSession::ToGeos(const Value &value) {
	arena.Reset();
	auto &blob = StringValue::Get(value);
	const auto geom = GeosSerde::Deserialize(ctx, arena, blob.data(), blob.size());
	if (!geom) {
		throw InvalidInputException("Could not deserialize geometry");
	}
	return GeosGeometry(ctx, geom);
}

Value TopoSession::ToValue(const GEOSGeometry *geom) const {
	const auto size = GeosSerde::GetRequiredSize(ctx, geom);
	string buffer(size, '\0');
	GeosSerde::Serialize(ctx, geom, &buffer[0], size);
	return Value::GEOMETRY(const_data_ptr_cast(buffer.data()), size);
}

GeosGeometry TopoSession::MakePoint(const TopoPoint &p) const {
	return GeosGeometry(ctx, GEOSGeom_createPointFromXY_r(ctx, p.x, p.y));
}

GeosGeometry TopoSession::MakeLine(const vector<TopoPoint> &pts) const {
	static_assert(sizeof(TopoPoint) == 2 * sizeof(double), "TopoPoint must be two packed doubles");
	const auto seq = GEOSCoordSeq_copyFromBuffer_r(ctx, reinterpret_cast<const double *>(pts.data()),
	                                               UnsafeNumericCast<unsigned int>(pts.size()), 0, 0);
	return GeosGeometry(ctx, GEOSGeom_createLineString_r(ctx, seq));
}

Value TopoSession::PointValue(const TopoPoint &p) const {
	const auto point = MakePoint(p);
	return ToValue(point.get_raw());
}

Value TopoSession::LineValue(const vector<TopoPoint> &pts) const {
	const auto line = MakeLine(pts);
	return ToValue(line.get_raw());
}

Value TopoSession::BoxValue(const TopoBox &box) const {
	if (box.IsEmpty()) {
		return Value(LogicalType::GEOMETRY());
	}
	const double coords[10] = {box.xmin, box.ymin, box.xmax, box.ymin, box.xmax,
	                           box.ymax, box.xmin, box.ymax, box.xmin, box.ymin};
	const auto seq = GEOSCoordSeq_copyFromBuffer_r(ctx, coords, 5, 0, 0);
	const auto ring = GEOSGeom_createLinearRing_r(ctx, seq);
	const GeosGeometry polygon(ctx, GEOSGeom_createPolygon_r(ctx, ring, nullptr, 0));
	return ToValue(polygon.get_raw());
}

vector<TopoPoint> TopoSession::Coordinates(const GEOSGeometry *geom) const {
	vector<TopoPoint> result;
	if (GEOSisEmpty_r(ctx, geom)) {
		return result;
	}
	const auto seq = GEOSGeom_getCoordSeq_r(ctx, geom);
	if (!seq) {
		return result;
	}
	unsigned int size = 0;
	GEOSCoordSeq_getSize_r(ctx, seq, &size);
	result.resize(size);
	if (size > 0) {
		GEOSCoordSeq_copyToBuffer_r(ctx, seq, reinterpret_cast<double *>(result.data()), 0, 0);
	}
	return result;
}

vector<TopoPoint> TopoSession::Coordinates(const Value &value) {
	if (value.IsNull()) {
		return vector<TopoPoint>();
	}
	const auto geom = ToGeos(value);
	return Coordinates(geom.get_raw());
}

TopoBox TopoSession::BoxOf(const Value &value) {
	TopoBox box;
	if (value.IsNull()) {
		return box;
	}
	const auto geom = ToGeos(value);
	if (geom.is_empty()) {
		return box;
	}
	geom.get_extent(box.xmin, box.ymin, box.xmax, box.ymax);
	return box;
}

//------------------------------------------------------------------------------
// Data access
//------------------------------------------------------------------------------

Value TopoSession::IdList(const vector<int32_t> &ids) {
	vector<Value> values;
	values.reserve(ids.size());
	for (auto id : ids) {
		values.push_back(Value::INTEGER(id));
	}
	return Value::LIST(LogicalType::INTEGER, std::move(values));
}

vector<TopoEdge> TopoSession::LoadEdges(const string &where, vector<Value> params) {
	auto result =
	    Query(string("SELECT ") + EDGE_COLUMNS + " FROM " + Table("edge_data") + " WHERE " + where, std::move(params));
	vector<TopoEdge> edges;
	edges.reserve(result->RowCount());
	ForEachRow(*result, [&](DataChunk &chunk, idx_t row) {
		TopoEdge edge;
		edge.id = chunk.GetValue(0, row).GetValue<int32_t>();
		edge.start_node = chunk.GetValue(1, row).GetValue<int32_t>();
		edge.end_node = chunk.GetValue(2, row).GetValue<int32_t>();
		edge.next_left = chunk.GetValue(3, row).GetValue<int32_t>();
		edge.next_right = chunk.GetValue(4, row).GetValue<int32_t>();
		edge.left_face = chunk.GetValue(5, row).GetValue<int32_t>();
		edge.right_face = chunk.GetValue(6, row).GetValue<int32_t>();
		edge.pts = Coordinates(chunk.GetValue(7, row));
		edges.push_back(std::move(edge));
	});
	return edges;
}

vector<TopoNode> TopoSession::LoadNodes(const string &where, vector<Value> params) {
	auto result =
	    Query("SELECT node_id, containing_face, geom FROM " + Table("node") + " WHERE " + where, std::move(params));
	vector<TopoNode> nodes;
	nodes.reserve(result->RowCount());
	ForEachRow(*result, [&](DataChunk &chunk, idx_t row) {
		TopoNode node;
		node.id = chunk.GetValue(0, row).GetValue<int32_t>();
		const auto face = chunk.GetValue(1, row);
		node.has_face = !face.IsNull();
		node.face = node.has_face ? face.GetValue<int32_t>() : 0;
		const auto pts = Coordinates(chunk.GetValue(2, row));
		if (!pts.empty()) {
			node.pt = pts[0];
		}
		nodes.push_back(node);
	});
	return nodes;
}

vector<TopoFace> TopoSession::LoadFaces(const string &where, vector<Value> params) {
	auto result = Query("SELECT face_id, mbr FROM " + Table("face") + " WHERE " + where, std::move(params));
	vector<TopoFace> faces;
	faces.reserve(result->RowCount());
	ForEachRow(*result, [&](DataChunk &chunk, idx_t row) {
		TopoFace face;
		face.id = chunk.GetValue(0, row).GetValue<int32_t>();
		face.mbr = BoxOf(chunk.GetValue(1, row));
		face.has_mbr = !face.mbr.IsEmpty();
		faces.push_back(face);
	});
	return faces;
}

static vector<Value> BoxParams(const TopoBox &box) {
	return {Value::DOUBLE(box.xmin), Value::DOUBLE(box.ymin), Value::DOUBLE(box.xmax), Value::DOUBLE(box.ymax)};
}

vector<TopoEdge> TopoSession::LoadEdgesInBox(const TopoBox &box) {
	return LoadEdges("geom && ST_MakeEnvelope(?, ?, ?, ?)", BoxParams(box));
}

vector<TopoNode> TopoSession::LoadNodesInBox(const TopoBox &box) {
	return LoadNodes("geom && ST_MakeEnvelope(?, ?, ?, ?)", BoxParams(box));
}

bool TopoSession::LoadNode(int32_t id, TopoNode &node) {
	auto nodes = LoadNodes("node_id = ?", {Value::INTEGER(id)});
	if (nodes.empty()) {
		return false;
	}
	node = nodes[0];
	return true;
}

bool TopoSession::LoadEdge(int32_t id, TopoEdge &edge) {
	auto edges = LoadEdges("edge_id = ?", {Value::INTEGER(id)});
	if (edges.empty()) {
		return false;
	}
	edge = std::move(edges[0]);
	return true;
}

bool TopoSession::LoadFace(int32_t id, TopoFace &face) {
	auto faces = LoadFaces("face_id = ?", {Value::INTEGER(id)});
	if (faces.empty()) {
		return false;
	}
	face = faces[0];
	return true;
}

vector<int32_t> TopoSession::NextIds(const char *sequence, idx_t count) {
	vector<int32_t> ids;
	if (count == 0) {
		return ids;
	}
	const auto name = Quote(catalog) + "." + Quote(info.name) + "." + sequence;
	auto result = Query("SELECT nextval(" + KeywordHelper::WriteQuoted(name, '\'') + ") FROM range(?)",
	                    {Value::BIGINT(NumericCast<int64_t>(count))});
	ids.reserve(count);
	ForEachRow(*result, [&](DataChunk &chunk, idx_t row) {
		const auto id = chunk.GetValue(0, row).GetValue<int64_t>();
		if (id > NumericLimits<int32_t>::Maximum()) {
			throw InvalidInputException("Topology identifier sequence %s is exhausted", sequence);
		}
		ids.push_back(UnsafeNumericCast<int32_t>(id));
	});
	std::sort(ids.begin(), ids.end());
	return ids;
}

int32_t TopoSession::NextId(const char *sequence) {
	return NextIds(sequence, 1)[0];
}

void TopoSession::InsertNode(const TopoNode &node) {
	Query("INSERT INTO " + Table("node") + "(node_id, containing_face, geom) VALUES (?, ?, ?)",
	      {Value::INTEGER(node.id), node.has_face ? Value::INTEGER(node.face) : Value(LogicalType::INTEGER),
	       PointValue(node.pt)});
}

void TopoSession::InsertFace(int32_t id, const TopoBox &mbr) {
	Query("INSERT INTO " + Table("face") + "(face_id, mbr) VALUES (?, ?)", {Value::INTEGER(id), BoxValue(mbr)});
}

void TopoSession::UpdateFaceBox(int32_t id, const TopoBox &mbr) {
	Query("UPDATE " + Table("face") + " SET mbr = ? WHERE face_id = ?", {BoxValue(mbr), Value::INTEGER(id)});
}

void TopoSession::SetContainingFace(const vector<int32_t> &nodes, const Value &face) {
	if (nodes.empty()) {
		return;
	}
	Query("UPDATE " + Table("node") + " SET containing_face = ? WHERE list_contains(?, node_id)",
	      {face.DefaultCastAs(LogicalType::INTEGER), IdList(nodes)});
}

void TopoSession::RefreshFaceBox(int32_t face) {
	if (face == 0) {
		return;
	}
	TopoBox box;
	for (auto &edge : LoadEdges("(left_face = ?) <> (right_face = ?)", {Value::INTEGER(face), Value::INTEGER(face)})) {
		box.Add(edge.pts);
	}
	if (!box.IsEmpty()) {
		UpdateFaceBox(face, box);
	}
}

int32_t TopoSession::FaceContainingPoint(const TopoPoint &p) {
	auto result = Query("SELECT face_id FROM " + Table("face") + " WHERE face_id <> 0 AND mbr && ST_Point(?, ?)",
	                    {Value::DOUBLE(p.x), Value::DOUBLE(p.y)});
	vector<int32_t> candidates;
	ForEachRow(*result,
	           [&](DataChunk &chunk, idx_t row) { candidates.push_back(chunk.GetValue(0, row).GetValue<int32_t>()); });
	if (candidates.empty()) {
		return 0;
	}
	const auto list = IdList(candidates);
	const auto edges = LoadEdges("list_contains(?, left_face) OR list_contains(?, right_face)", {list, list});
	unordered_map<int32_t, idx_t> crossings;
	for (auto &edge : edges) {
		if (edge.left_face == edge.right_face) {
			continue;
		}
		const auto count = CrossingCount(edge.pts, p);
		crossings[edge.left_face] += count;
		crossings[edge.right_face] += count;
	}
	for (auto face : candidates) {
		if ((crossings[face] & 1) == 1) {
			return face;
		}
	}
	return 0;
}

void TopoSession::CheckNodeLocation(const TopoPoint &p) {
	TopoBox box;
	box.Add(p);
	for (auto &node : LoadNodesInBox(box)) {
		if (node.pt == p) {
			throw InvalidInputException("SQL/MM Spatial exception - coincident node");
		}
	}
	for (auto &edge : LoadEdgesInBox(box)) {
		idx_t segment;
		bool at_vertex;
		if (LocatePointOnLine(edge.pts, p, segment, at_vertex)) {
			throw InvalidInputException("SQL/MM Spatial exception - edge crosses node.");
		}
	}
}

//------------------------------------------------------------------------------
// BoxIndex
//------------------------------------------------------------------------------

BoxIndex::BoxIndex(GEOSContextHandle_t ctx_p) : ctx(ctx_p), tree(GEOSSTRtree_create_r(ctx_p, 10)) {
}

BoxIndex::~BoxIndex() {
	GEOSSTRtree_destroy_r(ctx, tree);
	for (auto envelope : envelopes) {
		GEOSGeom_destroy_r(ctx, envelope);
	}
}

static GEOSGeometry *MakeDiagonal(GEOSContextHandle_t ctx, const TopoBox &box) {
	const double coords[4] = {box.xmin, box.ymin, box.xmax, box.ymax};
	const auto seq = GEOSCoordSeq_copyFromBuffer_r(ctx, coords, 2, 0, 0);
	return GEOSGeom_createLineString_r(ctx, seq);
}

void BoxIndex::Insert(const TopoBox &box, idx_t item) {
	const auto envelope = MakeDiagonal(ctx, box);
	envelopes.push_back(envelope);
	GEOSSTRtree_insert_r(ctx, tree, envelope, reinterpret_cast<void *>(item + 1));
}

void BoxIndex::Query(const TopoBox &box, vector<idx_t> &result) {
	result.clear();
	const auto envelope = MakeDiagonal(ctx, box);
	GEOSSTRtree_query_r(
	    ctx, tree, envelope,
	    [](void *item, void *data) {
		    static_cast<vector<idx_t> *>(data)->push_back(reinterpret_cast<idx_t>(item) - 1);
	    },
	    &result);
	GEOSGeom_destroy_r(ctx, envelope);
}

} // namespace topology

} // namespace duckdb
