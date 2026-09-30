#pragma once

#include "duckdb/common/mutex.hpp"
#include "duckdb/common/types/value.hpp"
#include "duckdb/main/connection.hpp"
#include "duckdb/main/materialized_query_result.hpp"
#include "duckdb/main/prepared_statement.hpp"
#include "duckdb/storage/arena_allocator.hpp"

#include "spatial/modules/geos/geos_geometry.hpp"
#include "spatial/modules/topology/topology_graph.hpp"

namespace duckdb {

namespace topology {

struct TopoInfo {
	int32_t id = 0;
	string name;
	int32_t srid = 0;
	double precision = 0;
};

class TopologyLock;

//! All access to the topology tables of one function call: a dedicated connection on the caller's database running a
//! single transaction, which is rolled back unless Commit() is reached
class TopoSession {
public:
	TopoSession(ClientContext &context, bool write);
	~TopoSession();

	void Commit();

	unique_ptr<MaterializedQueryResult> Query(const string &sql, vector<Value> params = vector<Value>());
	void Execute(const string &sql);

	template <class FUN>
	void ForEachRow(MaterializedQueryResult &result, FUN &&fun) {
		for (auto &chunk : result.Collection().Chunks()) {
			for (idx_t row = 0; row < chunk.size(); row++) {
				fun(chunk, row);
			}
		}
	}

	//! "SQL/MM Spatial exception - invalid topology name" unless the topology is registered
	void OpenTopology(const Value &name);
	bool TryOpenTopology(const string &name);
	bool RegistryExists();
	void CreateRegistry();
	void CheckSRID(const string &crs) const;

	const TopoInfo &Info() const {
		return info;
	}
	const string &Catalog() const {
		return catalog;
	}
	string Registry() const;
	string Table(const char *table) const;
	static string SchemaFor(const string &catalog, const string &name);
	static void CheckName(const string &name);

	GEOSContextHandle_t Geos() const {
		return ctx;
	}
	GeosGeometry ToGeos(const Value &value);
	Value ToValue(const GEOSGeometry *geom) const;
	GeosGeometry MakePoint(const TopoPoint &p) const;
	GeosGeometry MakeLine(const vector<TopoPoint> &pts) const;
	Value PointValue(const TopoPoint &p) const;
	Value LineValue(const vector<TopoPoint> &pts) const;
	Value BoxValue(const TopoBox &box) const;
	vector<TopoPoint> Coordinates(const GEOSGeometry *geom) const;
	vector<TopoPoint> Coordinates(const Value &value);
	TopoBox BoxOf(const Value &value);

	vector<TopoEdge> LoadEdges(const string &where, vector<Value> params = vector<Value>());
	vector<TopoNode> LoadNodes(const string &where, vector<Value> params = vector<Value>());
	vector<TopoFace> LoadFaces(const string &where, vector<Value> params = vector<Value>());
	vector<TopoEdge> LoadEdgesInBox(const TopoBox &box);
	vector<TopoNode> LoadNodesInBox(const TopoBox &box);
	bool LoadNode(int32_t id, TopoNode &node);
	bool LoadEdge(int32_t id, TopoEdge &edge);
	bool LoadFace(int32_t id, TopoFace &face);

	vector<int32_t> NextIds(const char *sequence, idx_t count);
	int32_t NextId(const char *sequence);

	void InsertNode(const TopoNode &node);
	void InsertFace(int32_t id, const TopoBox &mbr);
	void UpdateFaceBox(int32_t id, const TopoBox &mbr);
	void SetContainingFace(const vector<int32_t> &nodes, const Value &face);
	//! Recompute the bounding box of a face from the edges that bound it
	void RefreshFaceBox(int32_t face);

	//! Face whose interior holds p, 0 for the universal face. p must not lie on an edge
	int32_t FaceContainingPoint(const TopoPoint &p);
	//! Raises "coincident node" / "edge crosses node." if p cannot host a new node
	void CheckNodeLocation(const TopoPoint &p);

	static Value IdList(const vector<int32_t> &ids);

private:
	[[noreturn]] void Rethrow(const ErrorData &error) const;
	//! An edit is invisible to, and blind to, the uncommitted work of the transaction calling it
	void CheckCallerTransaction() const;

	ClientContext &context;
	Connection con;
	shared_ptr<TopologyLock> lock_entry;
	unique_lock<mutex> write_lock;
	bool active = false;
	string catalog;
	TopoInfo info;
	string schema;
	GEOSContextHandle_t ctx;
	ArenaAllocator arena;
	unordered_map<string, unique_ptr<PreparedStatement>> statements;
};

//! Spatial lookup over bounding boxes backed by a GEOS STRtree
class BoxIndex {
public:
	explicit BoxIndex(GEOSContextHandle_t ctx);
	~BoxIndex();
	BoxIndex(const BoxIndex &) = delete;
	BoxIndex &operator=(const BoxIndex &) = delete;

	void Insert(const TopoBox &box, idx_t item);
	void Query(const TopoBox &box, vector<idx_t> &result);

private:
	GEOSContextHandle_t ctx;
	GEOSSTRtree *tree;
	vector<GEOSGeometry *> envelopes;
};

} // namespace topology

} // namespace duckdb
