#include "spatial/geometry/bbox.hpp"
#include "spatial/geometry/geometry_serialization.hpp"
#include "spatial/geometry/sgl.hpp"
#include "spatial/modules/main/spatial_functions.hpp"
#include "spatial/spatial_types.hpp"
#include "spatial/util/function_builder.hpp"

namespace duckdb {

namespace {

//######################################################################################################################
// Table Functions
//######################################################################################################################

//======================================================================================================================
// ST_GeneratePoints
//======================================================================================================================

struct ST_GeneratePoints {

	//------------------------------------------------------------------------------------------------------------------
	// Bind
	//------------------------------------------------------------------------------------------------------------------
	struct GeneratePointsBindData final : TableFunctionData {
		idx_t count = 0;
		int64_t seed = -1;
		Box2D<double> bbox;
	};

	static unique_ptr<FunctionData> Bind(ClientContext &context, TableFunctionBindInput &input,
	                                     vector<LogicalType> &return_types, vector<string> &names) {
		auto result = make_uniq<GeneratePointsBindData>();

		return_types.push_back(GeoTypes::POINT_2D());
		names.push_back("point");

		// Extract the bounding box
		const auto &box_value = input.inputs[0];
		auto &box_components = StructValue::GetChildren(box_value);
		result->bbox.min.x = box_components[0].GetValue<double>();
		result->bbox.min.y = box_components[1].GetValue<double>();
		result->bbox.max.x = box_components[2].GetValue<double>();
		result->bbox.max.y = box_components[3].GetValue<double>();

		// Extract the count
		const auto &count_value = input.inputs[1];
		const auto count = count_value.GetValue<int64_t>();
		if (count < 0) {
			throw BinderException("Count must be a non-negative integer");
		}
		result->count = UnsafeNumericCast<idx_t>(count);

		// Extract the seed (optional)
		if (input.inputs.size() == 3) {
			result->seed = input.inputs[2].GetValue<int64_t>();
		}

		return std::move(result);
	}

	//------------------------------------------------------------------------------------------------------------------
	// Init
	//------------------------------------------------------------------------------------------------------------------
	struct GeneratePointsState final : GlobalTableFunctionState {
		RandomEngine rng;
		idx_t current_idx;

		explicit GeneratePointsState(const int64_t seed) : rng(seed), current_idx(0) {
		}
	};

	static unique_ptr<GlobalTableFunctionState> Init(ClientContext &context, TableFunctionInitInput &input) {
		auto &bind_data = input.bind_data->Cast<GeneratePointsBindData>();
		auto result = make_uniq<GeneratePointsState>(bind_data.seed);
		return std::move(result);
	}

	//------------------------------------------------------------------------------------------------------------------
	// Execute
	//------------------------------------------------------------------------------------------------------------------
	static void Execute(ClientContext &context, TableFunctionInput &data_p, DataChunk &output) {
		auto &bind_data = data_p.bind_data->Cast<GeneratePointsBindData>();
		auto &state = data_p.global_state->Cast<GeneratePointsState>();

		const auto &point_vec = StructVector::GetEntries(output.data[0]);
		const auto &x_data = FlatVector::GetData<double>(*point_vec[0]);
		const auto &y_data = FlatVector::GetData<double>(*point_vec[1]);

		const auto chunk_size = MinValue<idx_t>(STANDARD_VECTOR_SIZE, bind_data.count - state.current_idx);
		for (idx_t i = 0; i < chunk_size; i++) {

			x_data[i] = state.rng.NextRandom32(bind_data.bbox.min.x, bind_data.bbox.max.x);
			y_data[i] = state.rng.NextRandom32(bind_data.bbox.min.y, bind_data.bbox.max.y);

			state.current_idx++;
		}
		output.SetCardinality(chunk_size);
	}

	//------------------------------------------------------------------------------------------------------------------
	// Cardinality
	//------------------------------------------------------------------------------------------------------------------
	static unique_ptr<NodeStatistics> Cardinality(ClientContext &context, const FunctionData *bind_data_p) {
		auto &bind_data = bind_data_p->Cast<GeneratePointsBindData>();
		return make_uniq<NodeStatistics>(bind_data.count, bind_data.count);
	}

	//------------------------------------------------------------------------------------------------------------------
	// DOCUMENTATION
	//------------------------------------------------------------------------------------------------------------------
	static constexpr auto DESCRIPTION = R"(
		Generates a set of random points within the specified bounding box.

		Takes a bounding box (min_x, min_y, max_x, max_y), a count of points to generate, and optionally a seed for the random number generator.
	)";
	static constexpr auto EXAMPLE =
	    "SELECT * FROM ST_GeneratePoints({min_x: 0, min_y:0, max_x:10, max_y:10}::BOX_2D, 5, 42);";

	//------------------------------------------------------------------------------------------------------------------
	// Register
	//------------------------------------------------------------------------------------------------------------------
	static void Register(ExtensionLoader &loader) {
		// TODO: Dont overload, make seed named parameter instead
		TableFunctionSet set("ST_GeneratePoints");

		TableFunction generate_points({GeoTypes::BOX_2D(), LogicalType::BIGINT}, Execute, Bind, Init);
		generate_points.cardinality = Cardinality;

		// Overload without seed
		set.AddFunction(generate_points);

		// Overload with seed
		generate_points.arguments = {GeoTypes::BOX_2D(), LogicalType::BIGINT, LogicalType::BIGINT};
		set.AddFunction(generate_points);
		loader.RegisterFunction(set);

		InsertionOrderPreservingMap<string> tags;
		tags.insert("ext", "spatial");
		FunctionBuilder::AddTableFunctionDocs(loader, "ST_GeneratePoints", DESCRIPTION, EXAMPLE, tags);
	}
};

//======================================================================================================================
// ST_SquareGrid
//======================================================================================================================

struct ST_SquareGrid {

	//------------------------------------------------------------------------------------------------------------------
	// Bind
	//------------------------------------------------------------------------------------------------------------------
	struct SquareGridBindData final : TableFunctionData {
		double cell_size = 0;
		// Grid-aligned cell index range (inclusive)
		int64_t i_min = 0;
		int64_t i_max = 0;
		int64_t j_min = 0;
		int64_t j_max = 0;
		// Total number of cells
		idx_t total_cells = 0;
	};

	static unique_ptr<FunctionData> Bind(ClientContext &context, TableFunctionBindInput &input,
	                                     vector<LogicalType> &return_types, vector<string> &names) {
		auto result = make_uniq<SquareGridBindData>();

		// Return columns: geom (GEOMETRY), i (BIGINT), j (BIGINT)
		return_types.push_back(LogicalType::GEOMETRY());
		names.push_back("geom");
		return_types.push_back(LogicalType::BIGINT);
		names.push_back("i");
		return_types.push_back(LogicalType::BIGINT);
		names.push_back("j");

		// Extract cell size
		const auto cell_size = input.inputs[0].GetValue<double>();
		if (cell_size <= 0 || !std::isfinite(cell_size)) {
			throw BinderException("ST_SquareGrid: cell size must be a positive finite number");
		}
		result->cell_size = cell_size;

		// Extract bbox from geometry input
		const auto &geom_blob = input.inputs[1].GetValueUnsafe<string_t>();

		ArenaAllocator arena(Allocator::DefaultAllocator());
		sgl::geometry geom;
		Serde::Deserialize(geom, arena, geom_blob.GetDataUnsafe(), geom_blob.GetSize());

		if (geom.is_empty()) {
			result->total_cells = 0;
			return std::move(result);
		}

		// Compute extent by visiting all vertices
		double min_x = std::numeric_limits<double>::max();
		double min_y = std::numeric_limits<double>::max();
		double max_x = std::numeric_limits<double>::lowest();
		double max_y = std::numeric_limits<double>::lowest();

		struct extent_state_t {
			double &min_x;
			double &min_y;
			double &max_x;
			double &max_y;
		} ext_state = {min_x, min_y, max_x, max_y};

		sgl::ops::visit_vertices_xy(geom, &ext_state, [](void *state_ptr, const sgl::vertex_xy &vertex) {
			auto &s = *static_cast<extent_state_t *>(state_ptr);
			s.min_x = std::min(s.min_x, vertex.x);
			s.min_y = std::min(s.min_y, vertex.y);
			s.max_x = std::max(s.max_x, vertex.x);
			s.max_y = std::max(s.max_y, vertex.y);
		});

		// Align grid to global coordinates (multiples of cell_size)
		// PostGIS behavior: grid origin at (0,0), cell boundaries at multiples of size
		// Cell (i,j) covers the closed square [i*size, (i+1)*size] x [j*size, (j+1)*size]
		result->i_min = static_cast<int64_t>(std::floor(min_x / cell_size));
		result->j_min = static_cast<int64_t>(std::floor(min_y / cell_size));

		// Number of cells: ceil(max/size) - floor(min/size), at minimum 1 cell.
		// This correctly handles exact-boundary cases: when max falls on a cell boundary,
		// it belongs to the previous cell (the one whose right edge is at max).
		const auto ncols = std::max(static_cast<int64_t>(1),
		    static_cast<int64_t>(std::ceil(max_x / cell_size)) - static_cast<int64_t>(std::floor(min_x / cell_size)));
		const auto nrows = std::max(static_cast<int64_t>(1),
		    static_cast<int64_t>(std::ceil(max_y / cell_size)) - static_cast<int64_t>(std::floor(min_y / cell_size)));
		result->i_max = result->i_min + ncols - 1;
		result->j_max = result->j_min + nrows - 1;

		const auto cols = static_cast<idx_t>(ncols);
		const auto rows = static_cast<idx_t>(nrows);
		result->total_cells = cols * rows;

		return std::move(result);
	}

	//------------------------------------------------------------------------------------------------------------------
	// Init
	//------------------------------------------------------------------------------------------------------------------
	struct SquareGridState final : GlobalTableFunctionState {
		idx_t current_idx = 0;
		ArenaAllocator arena;

		explicit SquareGridState(Allocator &allocator) : arena(allocator) {
		}
	};

	static unique_ptr<GlobalTableFunctionState> Init(ClientContext &context, TableFunctionInitInput &input) {
		return make_uniq<SquareGridState>(BufferAllocator::Get(context));
	}

	//------------------------------------------------------------------------------------------------------------------
	// Execute
	//------------------------------------------------------------------------------------------------------------------
	static void Execute(ClientContext &context, TableFunctionInput &data_p, DataChunk &output) {
		auto &bind_data = data_p.bind_data->Cast<SquareGridBindData>();
		auto &state = data_p.global_state->Cast<SquareGridState>();

		const auto cell_size = bind_data.cell_size;
		const auto cols = static_cast<idx_t>(bind_data.i_max - bind_data.i_min + 1);

		const auto chunk_size = MinValue<idx_t>(STANDARD_VECTOR_SIZE, bind_data.total_cells - state.current_idx);

		auto &geom_vec = output.data[0];
		auto &i_vec = output.data[1];
		auto &j_vec = output.data[2];

		auto i_data = FlatVector::GetData<int64_t>(i_vec);
		auto j_data = FlatVector::GetData<int64_t>(j_vec);

		for (idx_t k = 0; k < chunk_size; k++) {
			const auto flat_idx = state.current_idx + k;

			// Row-major order: iterate j (rows) first, then i (columns)
			const auto col_offset = flat_idx % cols;
			const auto row_offset = flat_idx / cols;

			const auto i = bind_data.i_min + static_cast<int64_t>(col_offset);
			const auto j = bind_data.j_min + static_cast<int64_t>(row_offset);

			i_data[k] = i;
			j_data[k] = j;

			// Construct the square polygon for cell (i, j)
			const auto x_min = static_cast<double>(i) * cell_size;
			const auto y_min = static_cast<double>(j) * cell_size;
			const auto x_max = x_min + cell_size;
			const auto y_max = y_min + cell_size;

			// 5 vertices for a closed ring (CCW)
			const double buffer[10] = {x_min, y_min, x_max, y_min, x_max, y_max, x_min, y_max, x_min, y_min};

			sgl::geometry ring(sgl::geometry_type::LINESTRING, false, false);
			ring.set_vertex_array(buffer, 5);

			sgl::geometry poly(sgl::geometry_type::POLYGON, false, false);
			poly.append_part(&ring);

			// Serialize into the result vector
			const auto size = Serde::GetRequiredSize(poly);
			auto str = StringVector::EmptyString(geom_vec, size);
			Serde::Serialize(poly, str.GetDataWriteable(), size);
			str.Finalize();
			FlatVector::GetData<string_t>(geom_vec)[k] = str;
		}

		state.current_idx += chunk_size;
		output.SetCardinality(chunk_size);
	}

	//------------------------------------------------------------------------------------------------------------------
	// Cardinality
	//------------------------------------------------------------------------------------------------------------------
	static unique_ptr<NodeStatistics> Cardinality(ClientContext &context, const FunctionData *bind_data_p) {
		auto &bind_data = bind_data_p->Cast<SquareGridBindData>();
		return make_uniq<NodeStatistics>(bind_data.total_cells, bind_data.total_cells);
	}

	//------------------------------------------------------------------------------------------------------------------
	// Documentation
	//------------------------------------------------------------------------------------------------------------------
	static constexpr auto DESCRIPTION = R"(
		Generates a regular grid of square polygons covering the bounding box of the input geometry.

		Takes a cell size and a geometry whose bounding box defines the grid extent.
		The grid is aligned to global coordinates (multiples of size), so grid cell (0,0) always covers the origin.
		Returns rows of (geom, i, j) where geom is the square polygon and (i, j) are the column and row indices.
	)";
	static constexpr auto EXAMPLE =
	    "SELECT * FROM ST_SquareGrid(1.0, ST_MakeEnvelope(0, 0, 2, 2));";

	//------------------------------------------------------------------------------------------------------------------
	// Register
	//------------------------------------------------------------------------------------------------------------------
	static void Register(ExtensionLoader &loader) {
		TableFunction square_grid("ST_SquareGrid", {LogicalType::DOUBLE, LogicalType::GEOMETRY()}, Execute, Bind, Init);
		square_grid.cardinality = Cardinality;

		loader.RegisterFunction(square_grid);

		InsertionOrderPreservingMap<string> tags;
		tags.insert("ext", "spatial");
		FunctionBuilder::AddTableFunctionDocs(loader, "ST_SquareGrid", DESCRIPTION, EXAMPLE, tags);
	}
};

//======================================================================================================================
// ST_HexagonGrid
//======================================================================================================================

struct ST_HexagonGrid {

	//------------------------------------------------------------------------------------------------------------------
	// Bind
	//------------------------------------------------------------------------------------------------------------------
	struct HexagonGridBindData final : TableFunctionData {
		double size = 0;
		// Hex geometry constants
		double col_spacing = 0; // size * 1.5
		double row_spacing = 0; // size * sqrt(3)
		double half_height = 0; // size * sqrt(3) / 2
		// Grid-aligned cell index range (inclusive)
		int64_t i_min = 0;
		int64_t i_max = 0;
		int64_t j_min = 0;
		int64_t j_max = 0;
		// Total number of cells
		idx_t total_cells = 0;
	};

	static unique_ptr<FunctionData> Bind(ClientContext &context, TableFunctionBindInput &input,
	                                     vector<LogicalType> &return_types, vector<string> &names) {
		auto result = make_uniq<HexagonGridBindData>();

		// Return columns: geom (GEOMETRY), i (BIGINT), j (BIGINT)
		return_types.push_back(LogicalType::GEOMETRY());
		names.push_back("geom");
		return_types.push_back(LogicalType::BIGINT);
		names.push_back("i");
		return_types.push_back(LogicalType::BIGINT);
		names.push_back("j");

		// Extract edge length
		const auto size = input.inputs[0].GetValue<double>();
		if (size <= 0 || !std::isfinite(size)) {
			throw BinderException("ST_HexagonGrid: size must be a positive finite number");
		}
		result->size = size;

		// Compute hex grid constants
		const double col_spacing = size * 1.5;
		const double row_spacing = size * std::sqrt(3.0);
		const double half_height = row_spacing / 2.0;
		result->col_spacing = col_spacing;
		result->row_spacing = row_spacing;
		result->half_height = half_height;

		// Extract bbox from geometry input
		const auto &geom_blob = input.inputs[1].GetValueUnsafe<string_t>();

		ArenaAllocator arena(Allocator::DefaultAllocator());
		sgl::geometry geom;
		Serde::Deserialize(geom, arena, geom_blob.GetDataUnsafe(), geom_blob.GetSize());

		if (geom.is_empty()) {
			result->total_cells = 0;
			return std::move(result);
		}

		// Compute extent by visiting all vertices
		double min_x = std::numeric_limits<double>::max();
		double min_y = std::numeric_limits<double>::max();
		double max_x = std::numeric_limits<double>::lowest();
		double max_y = std::numeric_limits<double>::lowest();

		struct extent_state_t {
			double &min_x;
			double &min_y;
			double &max_x;
			double &max_y;
		} ext_state = {min_x, min_y, max_x, max_y};

		sgl::ops::visit_vertices_xy(geom, &ext_state, [](void *state_ptr, const sgl::vertex_xy &vertex) {
			auto &s = *static_cast<extent_state_t *>(state_ptr);
			s.min_x = std::min(s.min_x, vertex.x);
			s.min_y = std::min(s.min_y, vertex.y);
			s.max_x = std::max(s.max_x, vertex.x);
			s.max_y = std::max(s.max_y, vertex.y);
		});

		// Compute the range of column indices that cover the bbox
		// For column i, the hex center x = i * col_spacing
		// The hexagon extends from cx - size to cx + size in x
		// We need: cx + size >= min_x  =>  i >= (min_x - size) / col_spacing
		// We need: cx - size <= max_x  =>  i <= (max_x + size) / col_spacing
		result->i_min = static_cast<int64_t>(std::floor((min_x - size) / col_spacing));
		result->i_max = static_cast<int64_t>(std::floor((max_x + size) / col_spacing));

		// For row j, the hex center y depends on column parity:
		//   even columns: cy = j * row_spacing
		//   odd columns:  cy = j * row_spacing + half_height
		// The hexagon extends from cy - half_height to cy + half_height in y
		// Conservative bounds (covering both even and odd columns):
		result->j_min = static_cast<int64_t>(std::floor((min_y - half_height) / row_spacing)) - 1;
		result->j_max = static_cast<int64_t>(std::floor((max_y + half_height) / row_spacing)) + 1;

		const auto cols = static_cast<idx_t>(result->i_max - result->i_min + 1);
		const auto rows = static_cast<idx_t>(result->j_max - result->j_min + 1);
		result->total_cells = cols * rows;

		return std::move(result);
	}

	//------------------------------------------------------------------------------------------------------------------
	// Init
	//------------------------------------------------------------------------------------------------------------------
	struct HexagonGridState final : GlobalTableFunctionState {
		idx_t current_idx = 0;
		ArenaAllocator arena;

		explicit HexagonGridState(Allocator &allocator) : arena(allocator) {
		}
	};

	static unique_ptr<GlobalTableFunctionState> Init(ClientContext &context, TableFunctionInitInput &input) {
		return make_uniq<HexagonGridState>(BufferAllocator::Get(context));
	}

	//------------------------------------------------------------------------------------------------------------------
	// Execute
	//------------------------------------------------------------------------------------------------------------------
	static void Execute(ClientContext &context, TableFunctionInput &data_p, DataChunk &output) {
		auto &bind_data = data_p.bind_data->Cast<HexagonGridBindData>();
		auto &state = data_p.global_state->Cast<HexagonGridState>();

		const auto size = bind_data.size;
		const auto col_spacing = bind_data.col_spacing;
		const auto row_spacing = bind_data.row_spacing;
		const auto half_height = bind_data.half_height;
		const auto half_size = size / 2.0;
		const auto cols = static_cast<idx_t>(bind_data.i_max - bind_data.i_min + 1);

		const auto chunk_size = MinValue<idx_t>(STANDARD_VECTOR_SIZE, bind_data.total_cells - state.current_idx);

		auto &geom_vec = output.data[0];
		auto &i_vec = output.data[1];
		auto &j_vec = output.data[2];

		auto i_data = FlatVector::GetData<int64_t>(i_vec);
		auto j_data = FlatVector::GetData<int64_t>(j_vec);

		for (idx_t k = 0; k < chunk_size; k++) {
			const auto flat_idx = state.current_idx + k;

			// Row-major order: iterate columns first, then rows
			const auto col_offset = flat_idx % cols;
			const auto row_offset = flat_idx / cols;

			const auto i = bind_data.i_min + static_cast<int64_t>(col_offset);
			const auto j = bind_data.j_min + static_cast<int64_t>(row_offset);

			i_data[k] = i;
			j_data[k] = j;

			// Center of hexagon
			const double cx = static_cast<double>(i) * col_spacing;
			double cy = static_cast<double>(j) * row_spacing;
			// Odd columns are offset by half the row spacing
			if (((i % 2) + 2) % 2 == 1) {
				cy += half_height;
			}

			// Build the 7 vertices of the flat-top hexagon ring (first == last for closure)
			const double buffer[14] = {
			    cx + size,      cy,
			    cx + half_size,  cy + half_height,
			    cx - half_size,  cy + half_height,
			    cx - size,       cy,
			    cx - half_size,  cy - half_height,
			    cx + half_size,  cy - half_height,
			    cx + size,       cy  // close the ring
			};

			sgl::geometry ring(sgl::geometry_type::LINESTRING, false, false);
			ring.set_vertex_array(buffer, 7);

			sgl::geometry poly(sgl::geometry_type::POLYGON, false, false);
			poly.append_part(&ring);

			// Serialize into the result vector
			const auto blob_size = Serde::GetRequiredSize(poly);
			auto str = StringVector::EmptyString(geom_vec, blob_size);
			Serde::Serialize(poly, str.GetDataWriteable(), blob_size);
			str.Finalize();
			FlatVector::GetData<string_t>(geom_vec)[k] = str;
		}

		state.current_idx += chunk_size;
		output.SetCardinality(chunk_size);
	}

	//------------------------------------------------------------------------------------------------------------------
	// Cardinality
	//------------------------------------------------------------------------------------------------------------------
	static unique_ptr<NodeStatistics> Cardinality(ClientContext &context, const FunctionData *bind_data_p) {
		auto &bind_data = bind_data_p->Cast<HexagonGridBindData>();
		return make_uniq<NodeStatistics>(bind_data.total_cells, bind_data.total_cells);
	}

	//------------------------------------------------------------------------------------------------------------------
	// Documentation
	//------------------------------------------------------------------------------------------------------------------
	static constexpr auto DESCRIPTION = R"(
		Generates a regular hexagonal grid covering the bounding box of the input geometry.

		Takes an edge length (size) and a geometry whose bounding box defines the grid extent.
		The grid is aligned to global coordinates, with cell (0,0) centered at the origin.
		Returns rows of (geom, i, j) where geom is a flat-top hexagon polygon and (i, j) are the column and row indices.
		Odd columns are offset vertically by half the row spacing.
	)";
	static constexpr auto EXAMPLE =
	    "SELECT * FROM ST_HexagonGrid(1.0, ST_MakeEnvelope(0, 0, 3, 3));";

	//------------------------------------------------------------------------------------------------------------------
	// Register
	//------------------------------------------------------------------------------------------------------------------
	static void Register(ExtensionLoader &loader) {
		TableFunction hex_grid("ST_HexagonGrid", {LogicalType::DOUBLE, LogicalType::GEOMETRY()}, Execute, Bind, Init);
		hex_grid.cardinality = Cardinality;

		loader.RegisterFunction(hex_grid);

		InsertionOrderPreservingMap<string> tags;
		tags.insert("ext", "spatial");
		FunctionBuilder::AddTableFunctionDocs(loader, "ST_HexagonGrid", DESCRIPTION, EXAMPLE, tags);
	}
};

} // namespace

//######################################################################################################################
// Register
//######################################################################################################################
void RegisterSpatialTableFunctions(ExtensionLoader &loader) {
	ST_GeneratePoints::Register(loader);
	ST_SquareGrid::Register(loader);
	ST_HexagonGrid::Register(loader);
}

} // namespace duckdb
