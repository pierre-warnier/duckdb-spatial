#include "spatial/geometry/bbox.hpp"
#include "spatial/geometry/geometry_serialization.hpp"
#include "spatial/geometry/sgl.hpp"
#include "spatial/modules/main/spatial_functions.hpp"
#include "spatial/spatial_types.hpp"
#include "spatial/util/function_builder.hpp"

#include "duckdb/common/types/column/column_data_collection.hpp"
#include "duckdb/execution/expression_executor.hpp"
#include "duckdb/main/database.hpp"
#include "duckdb/optimizer/optimizer_extension.hpp"
#include "duckdb/planner/expression/bound_function_expression.hpp"
#include "duckdb/planner/expression/bound_window_expression.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <unordered_map>

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

// The clustering functions compute a value for every row of the partition up front, and the window callback only has
// to hand out the value of the current row. DuckDB does not tell the callback which row that is, only its frame, and
// callbacks run on several threads in no particular order. An optimizer rule therefore gives these functions the
// frame ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW, whose end is the position of the current row.
struct ClusterBindData : public FunctionData {
	bool has_row_frame = false;
};

static idx_t GetPartitionRow(AggregateInputData &aggr_input_data, const SubFrames &frames, const char *name) {
	if (!aggr_input_data.bind_data->Cast<ClusterBindData>().has_row_frame || frames.empty()) {
		throw InvalidInputException("%s cannot be used when the query optimizer or its extensions are disabled",
		                            name);
	}
	return frames.back().end - 1;
}

struct DBSCANBindData final : public ClusterBindData {
	double epsilon;
	int32_t min_points;

	DBSCANBindData(double eps, int32_t minpts) : epsilon(eps), min_points(minpts) {}

	unique_ptr<FunctionData> Copy() const override {
		auto result = make_uniq<DBSCANBindData>(epsilon, min_points);
		result->has_row_frame = has_row_frame;
		return std::move(result);
	}

	bool Equals(const FunctionData &other) const override {
		auto &o = other.Cast<DBSCANBindData>();
		return epsilon == o.epsilon && min_points == o.min_points && has_row_frame == o.has_row_frame;
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
	// Use double-precision coordinates to match GEOMETRY's underlying precision;
	// storing floats would round near the eps boundary and can flip cluster
	// membership for closely-spaced points or inputs with large coordinate
	// magnitudes (e.g., UTM eastings/northings in meters).
	struct PointEntry {
		double x, y;
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

			// Compute centroid via SGL, which handles all geometry types including
			// MULTI_POINT/MULTI_LINESTRING/MULTI_POLYGON/GEOMETRY_COLLECTION by
			// recursively walking the parts.
			sgl::vertex_xyzm centroid;
			if (geom.get_type() != sgl::geometry_type::INVALID &&
			    sgl::ops::get_centroid(geom, centroid)) {
				points[row_offset + i] = {centroid.x, centroid.y, true};
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
	// Grid coordinates are int64 rather than int32: with small eps and large
	// coordinates (e.g., mercator meters), floor(x / eps) can exceed the int32
	// range and casting an out-of-range double to int32 is undefined behavior.
	// The hash map is keyed on (gx, gy) so collisions don't merge distinct cells.
	const auto cell_size = epsilon > 0 ? epsilon : 1.0;
	const auto inv_cell = 1.0 / cell_size;
	const auto eps_sq = epsilon * epsilon;

	using GridKey = std::pair<int64_t, int64_t>;
	struct GridKeyHash {
		size_t operator()(const GridKey &k) const noexcept {
			auto mix = [](uint64_t v) {
				v ^= v >> 30;
				v *= 0xbf58476d1ce4e5b9ULL;
				v ^= v >> 27;
				v *= 0x94d049bb133111ebULL;
				v ^= v >> 31;
				return v;
			};
			return mix(static_cast<uint64_t>(k.first)) ^
			       (mix(static_cast<uint64_t>(k.second)) + 0x9e3779b97f4a7c15ULL);
		}
	};

	std::unordered_map<GridKey, vector<idx_t>, GridKeyHash> grid;
	for (idx_t i = 0; i < count; i++) {
		if (!points[i].valid) {
			continue;
		}
		auto gx = static_cast<int64_t>(std::floor(points[i].x * inv_cell));
		auto gy = static_cast<int64_t>(std::floor(points[i].y * inv_cell));
		grid[{gx, gy}].push_back(i);
	}

	auto find_neighbors = [&](idx_t row_idx) -> vector<idx_t> {
		vector<idx_t> neighbors;
		auto px = points[row_idx].x;
		auto py = points[row_idx].y;
		auto gx = static_cast<int64_t>(std::floor(px * inv_cell));
		auto gy = static_cast<int64_t>(std::floor(py * inv_cell));

		for (int64_t dx = -1; dx <= 1; dx++) {
			for (int64_t dy = -1; dy <= 1; dy++) {
				auto it = grid.find({gx + dx, gy + dy});
				if (it == grid.end()) {
					continue;
				}
				for (auto candidate : it->second) {
					auto cdx = px - points[candidate].x;
					auto cdy = py - points[candidate].y;
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

static void DBSCANWindow(AggregateInputData &aggr_input_data, const WindowPartitionInput &,
                         const_data_ptr_t g_state, data_ptr_t, const SubFrames &frames, Vector &result, idx_t rid) {

	auto &state = *reinterpret_cast<const DBSCANGlobalState *>(g_state);
	auto result_data = FlatVector::GetData<int32_t>(result);
	auto &result_validity = FlatVector::Validity(result);

	const auto partition_rid = GetPartitionRow(aggr_input_data, frames, "ST_ClusterDBSCAN");

	if (state.cluster_ids && partition_rid < state.count) {
		auto cluster_id = state.cluster_ids[partition_rid];
		if (cluster_id == DBSCAN_NOISE || cluster_id == DBSCAN_UNVISITED) {
			result_validity.SetInvalid(rid);
		} else {
			result_data[rid] = cluster_id;
		}
	} else {
		result_validity.SetInvalid(rid);
	}
}

// Unused callbacks — the window path uses window_init/window_callback and
// never drives update/combine/finalize. Leave these as no-ops rather than
// wired to the registration so the planner does not mistake the function for
// a regular aggregate and route it through a non-custom aggregator.
static void DBSCANUpdate(Vector[], AggregateInputData &, idx_t, Vector &, idx_t) {
}
static void DBSCANCombine(Vector &, Vector &, AggregateInputData &, idx_t) {
}
static void DBSCANFinalize(Vector &, AggregateInputData &, Vector &, idx_t, idx_t) {
}

//======================================================================================================================
// ST_ClusterKMeans
//======================================================================================================================

struct KMeansBindData final : public ClusterBindData {
	int32_t k;
	KMeansBindData(int32_t k_p) : k(k_p) {}
	unique_ptr<FunctionData> Copy() const override {
		auto result = make_uniq<KMeansBindData>(k);
		result->has_row_frame = has_row_frame;
		return std::move(result);
	}
	bool Equals(const FunctionData &other) const override {
		auto &o = other.Cast<KMeansBindData>();
		return k == o.k && has_row_frame == o.has_row_frame;
	}
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

	// Extract centroids (same pattern as DBSCAN). Double precision matches
	// GEOMETRY's underlying precision; float storage rounds coordinates and
	// can flip assignments/convergence for small distances or large magnitudes.
	struct Pt { double x, y; bool valid; };
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
			// Use SGL's centroid, which recurses through multi-part/collection types
			// (previous implementation treated every multi-part geometry as empty).
			sgl::vertex_xyzm centroid;
			if (geom.get_type() != sgl::geometry_type::INVALID &&
			    sgl::ops::get_centroid(geom, centroid)) {
				points[row_offset + i] = {centroid.x, centroid.y, true};
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

	// Deterministic farthest-first initialization: start from the first valid point,
	// then repeatedly pick the valid point whose nearest existing centroid is farthest
	// away. This is a deterministic variant of k-means++ seeding (no randomization)
	// chosen to keep results stable across runs and thread counts.
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

// See DBSCANWindow for the rid vs partition_rid explanation. Same indexing rule:
// monotonic counter in l_state, reset on g_state pointer change.
static void KMeansWindow(AggregateInputData &aggr_input_data, const WindowPartitionInput &,
                         const_data_ptr_t g_state, data_ptr_t, const SubFrames &frames, Vector &result, idx_t rid) {
	auto &state = *reinterpret_cast<const KMeansGlobalState *>(g_state);
	auto result_data = FlatVector::GetData<int32_t>(result);
	auto &result_validity = FlatVector::Validity(result);

	const auto partition_rid = GetPartitionRow(aggr_input_data, frames, "ST_ClusterKMeans");

	if (state.cluster_ids && partition_rid < state.count && state.cluster_ids[partition_rid] >= 0) {
		result_data[rid] = state.cluster_ids[partition_rid];
	} else {
		result_validity.SetInvalid(rid);
	}
}

//======================================================================================================================
// Row frame rule
//======================================================================================================================

static void SetClusterRowFrames(OptimizerExtensionInput &input, unique_ptr<LogicalOperator> &plan) {
	for (auto &child : plan->children) {
		SetClusterRowFrames(input, child);
	}
	if (plan->type != LogicalOperatorType::LOGICAL_WINDOW) {
		return;
	}
	for (auto &expr : plan->expressions) {
		if (expr->GetExpressionClass() != ExpressionClass::BOUND_WINDOW) {
			continue;
		}
		auto &window = expr->Cast<BoundWindowExpression>();
		if (!window.aggregate || !window.bind_info) {
			continue;
		}
		if (window.aggregate->window != DBSCANWindow && window.aggregate->window != KMeansWindow) {
			continue;
		}
		window.start = WindowBoundary::UNBOUNDED_PRECEDING;
		window.end = WindowBoundary::CURRENT_ROW_ROWS;
		window.start_expr = nullptr;
		window.end_expr = nullptr;
		window.exclude_clause = WindowExcludeMode::NO_OTHER;
		window.bind_info->Cast<ClusterBindData>().has_row_frame = true;
	}
}

} // namespace

//======================================================================================================================
// Register
//======================================================================================================================

void RegisterSpatialWindowFunctions(ExtensionLoader &loader) {

	// Before the built-in optimizers, so that none of them sees the whole-partition frame
	OptimizerExtension row_frame_rule;
	row_frame_rule.pre_optimize_function = SetClusterRowFrames;
	OptimizerExtension::Register(loader.GetDatabaseInstance().config, row_frame_rule);

	// ST_ClusterDBSCAN(geom, eps, minpoints) OVER (...)
	// The state holds the pre-computed cluster IDs for the entire partition.
	// wininit runs DBSCAN and fills the state; the window callback reads it.
	AggregateFunction dbscan_func(
	    "ST_ClusterDBSCAN",
	    {LogicalType::GEOMETRY(), LogicalType::DOUBLE, LogicalType::INTEGER},
	    LogicalType::INTEGER,
	    AggregateFunction::StateSize<DBSCANGlobalState>,
	    DBSCANInit<DBSCANGlobalState>,
	    nullptr,    // update — all three must be null so the planner picks
	    nullptr,    // combine — WindowCustomAggregator (CanAggregate() == false)
	    nullptr,    // finalize — instead of Constant/Segment aggregators
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

			The whole partition is always clustered: the frame clause and ORDER BY of the window do not affect the result.
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

			The whole partition is always clustered: the frame clause and ORDER BY of the window do not affect the result.
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
