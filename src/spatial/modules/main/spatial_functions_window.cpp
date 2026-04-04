#include "spatial/geometry/bbox.hpp"
#include "spatial/geometry/geometry_serialization.hpp"
#include "spatial/geometry/sgl.hpp"
#include "spatial/modules/main/spatial_functions.hpp"
#include "spatial/spatial_types.hpp"
#include "spatial/util/function_builder.hpp"

#include "duckdb/common/types/column/column_data_collection.hpp"
#include "duckdb/execution/expression_executor.hpp"
#include "duckdb/planner/expression/bound_function_expression.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <numeric>

namespace duckdb {

namespace {

//======================================================================================================================
// ST_ClusterDBSCAN
//======================================================================================================================
// Window function: ST_ClusterDBSCAN(geom, eps, minpoints) OVER (...)
// Returns an integer cluster ID per row, or NULL for noise points.
// Compatible with PostGIS ST_ClusterDBSCAN.

static constexpr int32_t DBSCAN_NOISE = -1;
static constexpr int32_t DBSCAN_UNVISITED = -2;

struct DBSCANBindData final : public FunctionData {
	double epsilon;
	int32_t min_points;

	DBSCANBindData(double eps, int32_t minpts) : epsilon(eps), min_points(minpts) {}

	unique_ptr<FunctionData> Copy() const override {
		return make_uniq<DBSCANBindData>(epsilon, min_points);
	}

	bool Equals(const FunctionData &other) const override {
		auto &o = other.Cast<DBSCANBindData>();
		return epsilon == o.epsilon && min_points == o.min_points;
	}
};

struct DBSCANGlobalState {
	// Pre-computed cluster IDs for every row in the partition.
	// Use a raw pointer instead of vector because DuckDB's aggregate state
	// machinery may memcpy the state struct, which breaks vector internals.
	int32_t *cluster_ids = nullptr;
	idx_t count = 0;

	void Allocate(idx_t n) {
		count = n;
		cluster_ids = new int32_t[n];
		for (idx_t i = 0; i < n; i++) {
			cluster_ids[i] = DBSCAN_UNVISITED;
		}
	}

	void Destroy() {
		delete[] cluster_ids;
		cluster_ids = nullptr;
		count = 0;
	}
};

static unique_ptr<FunctionData> DBSCANBind(ClientContext &context, AggregateFunction &function,
                                           vector<unique_ptr<Expression>> &arguments) {
	// eps and minpoints must be constant
	if (!arguments[1]->IsFoldable() || !arguments[2]->IsFoldable()) {
		throw InvalidInputException("ST_ClusterDBSCAN: eps and minpoints must be constant");
	}

	auto eps_val = ExpressionExecutor::EvaluateScalar(context, *arguments[1]).GetValue<double>();
	auto minpts_val = ExpressionExecutor::EvaluateScalar(context, *arguments[2]).GetValue<int32_t>();

	if (eps_val < 0) {
		throw InvalidInputException("ST_ClusterDBSCAN: eps must be >= 0, got %f", eps_val);
	}
	if (minpts_val < 1) {
		throw InvalidInputException("ST_ClusterDBSCAN: minpoints must be >= 1, got %d", minpts_val);
	}

	// Erase the constant arguments so only geometry is passed at runtime
	Function::EraseArgument(function, arguments, 2);
	Function::EraseArgument(function, arguments, 1);

	return make_uniq<DBSCANBindData>(eps_val, minpts_val);
}

// Initialize aggregate state (per-group in grouped context, but we use window)
template <class STATE>
static void DBSCANInit(const AggregateFunction &, data_ptr_t state) {
	new (state) STATE();
}

// Destructor — free heap-allocated cluster_ids
static void DBSCANDestroy(Vector &states, AggregateInputData &, idx_t count) {
	auto state_ptrs = FlatVector::GetData<data_ptr_t>(states);
	for (idx_t i = 0; i < count; i++) {
		reinterpret_cast<DBSCANGlobalState *>(state_ptrs[i])->Destroy();
	}
}

// Window initialization: runs DBSCAN on the entire partition
static void DBSCANWindowInit(AggregateInputData &aggr_input_data, const WindowPartitionInput &partition,
                             data_ptr_t g_state) {
	auto &state = *reinterpret_cast<DBSCANGlobalState *>(g_state);
	auto &bind_data = aggr_input_data.bind_data->Cast<DBSCANBindData>();

	const auto count = partition.count;
	const auto epsilon = bind_data.epsilon;
	const auto min_points = bind_data.min_points;

	state.Allocate(count);

	if (count == 0) {
		return;
	}

	// Phase 1: Extract centroids from the partition's geometry column
	struct PointEntry {
		float x, y;
		bool valid;
	};
	vector<PointEntry> points(count);

	// The partition inputs contain the runtime arguments.
	// Use the collection's own types for correct deserialization.
	DataChunk input_chunk;
	auto &col_types = partition.inputs->Types();
	vector<LogicalType> scan_types;
	for (auto &cid : partition.column_ids) {
		scan_types.push_back(col_types[cid]);
	}
	input_chunk.Initialize(Allocator::DefaultAllocator(), scan_types);

	ArenaAllocator arena(Allocator::DefaultAllocator());
	idx_t row_offset = 0;

	// Scan all rows from the collection
	ColumnDataScanState scan_state;
	vector<column_t> col_ids;
	for (auto &cid : partition.column_ids) {
		col_ids.push_back(cid);
	}
	partition.inputs->InitializeScan(scan_state, col_ids);

	while (partition.inputs->Scan(scan_state, input_chunk)) {
		input_chunk.Flatten();
		auto &geom_vec = input_chunk.data[0];
		auto geom_data = FlatVector::GetData<string_t>(geom_vec);
		auto &validity = FlatVector::Validity(geom_vec);

		for (idx_t i = 0; i < input_chunk.size(); i++) {
			if (!validity.RowIsValid(i)) {
				points[row_offset + i] = {0, 0, false};
				state.cluster_ids[row_offset + i] = DBSCAN_NOISE;
				continue;
			}

			auto &blob = geom_data[i];
			sgl::geometry geom;
			Serde::Deserialize(geom, arena, blob.GetDataUnsafe(), blob.GetSize());

			// Get centroid from vertex data
			auto vc = geom.is_multi_part() ? 0 : geom.get_vertex_count();
			if (geom.get_type() != sgl::geometry_type::INVALID && vc > 0) {
				double sx = 0, sy = 0;
				for (uint32_t v = 0; v < vc; v++) {
					auto vtx = geom.get_vertex_xy(v);
					sx += vtx.x;
					sy += vtx.y;
				}
				points[row_offset + i] = {static_cast<float>(sx / vc), static_cast<float>(sy / vc), true};
			} else {
				points[row_offset + i] = {0, 0, false};
				state.cluster_ids[row_offset + i] = DBSCAN_NOISE;
			}
			arena.Reset();
		}
		row_offset += input_chunk.size();
		input_chunk.Reset();
	}

	// Count valid points
	idx_t valid_count = 0;
	for (idx_t i = 0; i < count; i++) {
		if (points[i].valid) {
			valid_count++;
		}
	}

	if (valid_count == 0) {
		return;
	}

	// Phase 2: Build grid index for O(1) neighbor lookups
	// Grid cells of size epsilon — neighbors within eps are in the 3x3 surrounding cells.
	const auto cell_size = epsilon > 0 ? epsilon : 1.0;
	const auto inv_cell = 1.0 / cell_size;
	const auto eps_sq = epsilon * epsilon;

	auto make_key = [](int32_t gx, int32_t gy) -> uint64_t {
		return (static_cast<uint64_t>(static_cast<uint32_t>(gx)) << 32) | static_cast<uint32_t>(gy);
	};

	std::unordered_map<uint64_t, vector<idx_t>> grid;
	for (idx_t i = 0; i < count; i++) {
		if (!points[i].valid) {
			continue;
		}
		auto gx = static_cast<int32_t>(std::floor(static_cast<double>(points[i].x) * inv_cell));
		auto gy = static_cast<int32_t>(std::floor(static_cast<double>(points[i].y) * inv_cell));
		grid[make_key(gx, gy)].push_back(i);
	}

	auto find_neighbors = [&](idx_t row_idx) -> vector<idx_t> {
		vector<idx_t> neighbors;
		auto px = static_cast<double>(points[row_idx].x);
		auto py = static_cast<double>(points[row_idx].y);
		auto gx = static_cast<int32_t>(std::floor(px * inv_cell));
		auto gy = static_cast<int32_t>(std::floor(py * inv_cell));

		for (int32_t dx = -1; dx <= 1; dx++) {
			for (int32_t dy = -1; dy <= 1; dy++) {
				auto it = grid.find(make_key(gx + dx, gy + dy));
				if (it == grid.end()) {
					continue;
				}
				for (auto candidate : it->second) {
					auto cdx = px - static_cast<double>(points[candidate].x);
					auto cdy = py - static_cast<double>(points[candidate].y);
					if (cdx * cdx + cdy * cdy <= eps_sq) {
						neighbors.push_back(candidate);
					}
				}
			}
		}
		return neighbors;
	};

	// Phase 3: DBSCAN algorithm
	int32_t current_cluster = 0;

	// DBSCAN core loop
	for (idx_t i = 0; i < count; i++) {
		if (!points[i].valid || state.cluster_ids[i] != DBSCAN_UNVISITED) {
			continue;
		}

		auto neighbors = find_neighbors(i);

		if (static_cast<int32_t>(neighbors.size()) < min_points) {
			state.cluster_ids[i] = DBSCAN_NOISE;
			continue;
		}

		// Start new cluster
		state.cluster_ids[i] = current_cluster;

		// Expand cluster using a queue
		vector<idx_t> queue;
		for (auto n : neighbors) {
			if (n != i) {
				queue.push_back(n);
			}
		}

		idx_t queue_pos = 0;
		while (queue_pos < queue.size()) {
			auto q = queue[queue_pos++];

			if (state.cluster_ids[q] == DBSCAN_NOISE) {
				// Border point: was noise, now part of cluster
				state.cluster_ids[q] = current_cluster;
				continue;
			}

			if (state.cluster_ids[q] != DBSCAN_UNVISITED) {
				// Already assigned to a cluster
				continue;
			}

			state.cluster_ids[q] = current_cluster;

			auto q_neighbors = find_neighbors(q);
			if (static_cast<int32_t>(q_neighbors.size()) >= min_points) {
				// Core point: add its unvisited neighbors to the queue
				for (auto qn : q_neighbors) {
					if (state.cluster_ids[qn] == DBSCAN_UNVISITED || state.cluster_ids[qn] == DBSCAN_NOISE) {
						queue.push_back(qn);
					}
				}
			}
		}

		current_cluster++;
	}
}

// Per-row window callback: return the pre-computed cluster ID.
// The default frame for aggregate window functions is ROWS BETWEEN UNBOUNDED PRECEDING
// AND CURRENT ROW, so frames[0].end - 1 gives the partition row index.
static void DBSCANWindow(AggregateInputData &, const WindowPartitionInput &, const_data_ptr_t g_state,
                         data_ptr_t l_state, const SubFrames &frames, Vector &result, idx_t rid) {

	auto &state = *reinterpret_cast<const DBSCANGlobalState *>(g_state);
	auto result_data = FlatVector::GetData<int32_t>(result);
	auto &result_validity = FlatVector::Validity(result);

	if (state.cluster_ids && rid < state.count) {
		auto cluster_id = state.cluster_ids[rid];
		if (cluster_id == DBSCAN_NOISE || cluster_id == DBSCAN_UNVISITED) {
			result_validity.SetInvalid(rid);
		} else {
			result_data[rid] = cluster_id;
		}
	} else {
		result_validity.SetInvalid(rid);
	}
}

// Dummy callbacks required by the AggregateFunction API but unused for window-only functions
static void DBSCANUpdate(Vector[], AggregateInputData &, idx_t, Vector &, idx_t) {
}
static void DBSCANCombine(Vector &, Vector &, AggregateInputData &, idx_t) {
}
static void DBSCANFinalize(Vector &, AggregateInputData &, Vector &, idx_t, idx_t) {
}

//======================================================================================================================
// ST_ClusterKMeans
//======================================================================================================================

struct KMeansBindData final : public FunctionData {
	int32_t k;
	KMeansBindData(int32_t k_p) : k(k_p) {}
	unique_ptr<FunctionData> Copy() const override { return make_uniq<KMeansBindData>(k); }
	bool Equals(const FunctionData &other) const override { return k == other.Cast<KMeansBindData>().k; }
};

struct KMeansGlobalState {
	int32_t *cluster_ids = nullptr;
	idx_t count = 0;
	void Allocate(idx_t n) { count = n; cluster_ids = new int32_t[n]; }
	void Destroy() { delete[] cluster_ids; cluster_ids = nullptr; count = 0; }
};

static unique_ptr<FunctionData> KMeansBind(ClientContext &context, AggregateFunction &function,
                                           vector<unique_ptr<Expression>> &arguments) {
	if (!arguments[1]->IsFoldable()) {
		throw InvalidInputException("ST_ClusterKMeans: k must be a constant");
	}
	auto k_val = ExpressionExecutor::EvaluateScalar(context, *arguments[1]).GetValue<int32_t>();
	if (k_val < 1) {
		throw InvalidInputException("ST_ClusterKMeans: k must be >= 1, got %d", k_val);
	}
	Function::EraseArgument(function, arguments, 1);
	return make_uniq<KMeansBindData>(k_val);
}

static void KMeansWindowInit(AggregateInputData &aggr_input_data, const WindowPartitionInput &partition,
                             data_ptr_t g_state) {
	auto &state = *reinterpret_cast<KMeansGlobalState *>(g_state);
	auto &bind_data = aggr_input_data.bind_data->Cast<KMeansBindData>();

	const auto count = partition.count;
	const auto k = bind_data.k;
	state.Allocate(count);

	if (count == 0) return;

	// Extract centroids (same pattern as DBSCAN)
	struct Pt { float x, y; bool valid; };
	vector<Pt> points(count);

	auto &col_types = partition.inputs->Types();
	vector<LogicalType> scan_types;
	for (auto &cid : partition.column_ids) {
		scan_types.push_back(col_types[cid]);
	}
	DataChunk input_chunk;
	input_chunk.Initialize(Allocator::DefaultAllocator(), scan_types);
	ArenaAllocator arena(Allocator::DefaultAllocator());
	idx_t row_offset = 0;

	vector<column_t> col_ids;
	for (auto &cid : partition.column_ids) col_ids.push_back(cid);
	ColumnDataScanState scan_state;
	partition.inputs->InitializeScan(scan_state, col_ids);

	while (partition.inputs->Scan(scan_state, input_chunk)) {
		input_chunk.Flatten();
		auto geom_data = FlatVector::GetData<string_t>(input_chunk.data[0]);
		auto &validity = FlatVector::Validity(input_chunk.data[0]);

		for (idx_t i = 0; i < input_chunk.size(); i++) {
			if (!validity.RowIsValid(i)) {
				points[row_offset + i] = {0, 0, false};
				state.cluster_ids[row_offset + i] = -1; // NULL for invalid
				continue;
			}
			sgl::geometry geom;
			Serde::Deserialize(geom, arena, geom_data[i].GetDataUnsafe(), geom_data[i].GetSize());
			auto vc = geom.is_multi_part() ? 0 : geom.get_vertex_count();
			if (vc > 0) {
				double sx = 0, sy = 0;
				for (uint32_t v = 0; v < vc; v++) {
					auto vtx = geom.get_vertex_xy(v);
					sx += vtx.x; sy += vtx.y;
				}
				points[row_offset + i] = {static_cast<float>(sx / vc), static_cast<float>(sy / vc), true};
			} else {
				points[row_offset + i] = {0, 0, false};
				state.cluster_ids[row_offset + i] = -1;
			}
			arena.Reset();
		}
		row_offset += input_chunk.size();
		input_chunk.Reset();
	}

	// Collect valid point indices
	vector<idx_t> valid_indices;
	for (idx_t i = 0; i < count; i++) {
		if (points[i].valid) valid_indices.push_back(i);
	}
	if (valid_indices.empty()) return;

	auto effective_k = MinValue(static_cast<idx_t>(k), valid_indices.size());

	// K-means++ initialization: pick first centroid randomly, then furthest-first
	vector<double> cx(effective_k), cy(effective_k);
	cx[0] = points[valid_indices[0]].x;
	cy[0] = points[valid_indices[0]].y;

	for (idx_t c = 1; c < effective_k; c++) {
		// Find the point farthest from all existing centroids
		double best_dist = -1;
		idx_t best_idx = 0;
		for (auto vi : valid_indices) {
			double min_d = std::numeric_limits<double>::max();
			for (idx_t j = 0; j < c; j++) {
				double dx = points[vi].x - cx[j];
				double dy = points[vi].y - cy[j];
				min_d = std::min(min_d, dx * dx + dy * dy);
			}
			if (min_d > best_dist) {
				best_dist = min_d;
				best_idx = vi;
			}
		}
		cx[c] = points[best_idx].x;
		cy[c] = points[best_idx].y;
	}

	// Lloyd's algorithm: iterate until convergence
	vector<int32_t> assignments(count, -1);
	for (int iter = 0; iter < 100; iter++) {
		bool changed = false;

		// Assignment step
		for (auto vi : valid_indices) {
			double best_d = std::numeric_limits<double>::max();
			int32_t best_c = 0;
			for (idx_t c = 0; c < effective_k; c++) {
				double dx = points[vi].x - cx[c];
				double dy = points[vi].y - cy[c];
				double d = dx * dx + dy * dy;
				if (d < best_d) { best_d = d; best_c = static_cast<int32_t>(c); }
			}
			if (assignments[vi] != best_c) { changed = true; assignments[vi] = best_c; }
		}

		if (!changed) break;

		// Update step: recompute centroids
		vector<double> sum_x(effective_k, 0), sum_y(effective_k, 0);
		vector<idx_t> counts(effective_k, 0);
		for (auto vi : valid_indices) {
			auto c = assignments[vi];
			sum_x[c] += points[vi].x;
			sum_y[c] += points[vi].y;
			counts[c]++;
		}
		for (idx_t c = 0; c < effective_k; c++) {
			if (counts[c] > 0) {
				cx[c] = sum_x[c] / counts[c];
				cy[c] = sum_y[c] / counts[c];
			}
		}
	}

	// Store results
	for (idx_t i = 0; i < count; i++) {
		state.cluster_ids[i] = assignments[i];
	}
}

static void KMeansWindow(AggregateInputData &, const WindowPartitionInput &, const_data_ptr_t g_state,
                         data_ptr_t, const SubFrames &, Vector &result, idx_t rid) {
	auto &state = *reinterpret_cast<const KMeansGlobalState *>(g_state);
	auto result_data = FlatVector::GetData<int32_t>(result);
	auto &result_validity = FlatVector::Validity(result);

	if (state.cluster_ids && rid < state.count && state.cluster_ids[rid] >= 0) {
		result_data[rid] = state.cluster_ids[rid];
	} else {
		result_validity.SetInvalid(rid);
	}
}

} // namespace

//======================================================================================================================
// Register
//======================================================================================================================

void RegisterSpatialWindowFunctions(ExtensionLoader &loader) {

	// ST_ClusterDBSCAN(geom, eps, minpoints) OVER (...)
	// The state holds the pre-computed cluster IDs for the entire partition.
	// wininit runs DBSCAN and fills the state; the window callback reads it.
	AggregateFunction dbscan_func(
	    "ST_ClusterDBSCAN",
	    {LogicalType::GEOMETRY(), LogicalType::DOUBLE, LogicalType::INTEGER},
	    LogicalType::INTEGER,
	    AggregateFunction::StateSize<DBSCANGlobalState>,
	    DBSCANInit<DBSCANGlobalState>,
	    nullptr,    // update — force custom window path
	    nullptr,    // combine
	    nullptr,    // finalize
	    nullptr,    // simple_update
	    DBSCANBind,
	    DBSCANDestroy,
	    nullptr,    // statistics
	    DBSCANWindow
	);
	dbscan_func.window_init = DBSCANWindowInit;

	FunctionBuilder::RegisterAggregate(loader, "ST_ClusterDBSCAN", [&](AggregateFunctionBuilder &func) {
		func.SetFunction(dbscan_func);
		func.SetDescription(R"(
			Assigns a DBSCAN cluster ID to each geometry based on spatial proximity.
			Returns NULL for noise points. Must be used as a window function.

			Parameters:
			- geom: input geometry (centroid is used for distance)
			- eps: maximum distance between two points to be in the same neighborhood
			- minpoints: minimum number of points required to form a dense region

			Compatible with PostGIS ST_ClusterDBSCAN.
		)");
		func.SetExample(R"(
			SELECT ST_ClusterDBSCAN(geom, 5.0, 3) OVER () as cluster_id
			FROM my_points;
		)");
		func.CanThrowErrors();
		func.SetTag("ext", "spatial");
		func.SetTag("category", "clustering");
	});

	// ST_ClusterKMeans(geom, k) OVER (...)
	AggregateFunction kmeans_func(
	    "ST_ClusterKMeans",
	    {LogicalType::GEOMETRY(), LogicalType::INTEGER},
	    LogicalType::INTEGER,
	    AggregateFunction::StateSize<KMeansGlobalState>,
	    DBSCANInit<KMeansGlobalState>,
	    nullptr, nullptr, nullptr, nullptr,
	    KMeansBind,
	    [](Vector &states, AggregateInputData &, idx_t count) {
		    auto ptrs = FlatVector::GetData<data_ptr_t>(states);
		    for (idx_t i = 0; i < count; i++) {
			    reinterpret_cast<KMeansGlobalState *>(ptrs[i])->Destroy();
		    }
	    },
	    nullptr, KMeansWindow
	);
	kmeans_func.window_init = KMeansWindowInit;

	FunctionBuilder::RegisterAggregate(loader, "ST_ClusterKMeans", [&](AggregateFunctionBuilder &func) {
		func.SetFunction(kmeans_func);
		func.SetDescription(R"(
			Assigns a k-means cluster ID to each geometry.
			Returns integer cluster IDs (0 to k-1). Must be used as a window function.
			Compatible with PostGIS ST_ClusterKMeans.
		)");
		func.SetExample(R"(
			SELECT ST_ClusterKMeans(geom, 3) OVER () as cluster_id
			FROM my_points;
		)");
		func.CanThrowErrors();
		func.SetTag("ext", "spatial");
		func.SetTag("category", "clustering");
	});
}

} // namespace duckdb
