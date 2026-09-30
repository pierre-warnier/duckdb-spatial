#include "spatial/modules/raster/raster_core.hpp"
#include "spatial/modules/gdal/gdal_module.hpp"
#include "spatial/util/function_builder.hpp"

#include "duckdb/common/file_system.hpp"
#include "duckdb/function/table_function.hpp"

#include <atomic>

namespace duckdb {
namespace raster {

namespace {

// An empty, non-null sibling list keeps GDAL from listing the directory of the file
const char *const NO_SIBLINGS[] = {nullptr};

//======================================================================================================================
// ST_ReadRaster
//======================================================================================================================

struct RasterFile {
	string real_path;
	string gdal_path;
	int width;
	int height;
	idx_t tiles_x;
	idx_t tiles_y;
	idx_t first_tile;
};

struct ReadRasterBindData final : public TableFunctionData {
	vector<RasterFile> files;
	int tile_width = 0;
	int tile_height = 0;
	idx_t tile_count = 0;
};

GDALDatasetUniquePtr OpenRasterFile(const RasterFile &file) {
	CPLErrorReset();
	GDALDatasetUniquePtr ds(GDALDataset::Open(file.gdal_path.c_str(),
	                                          GDAL_OF_RASTER | GDAL_OF_READONLY | GDAL_OF_INTERNAL, nullptr, nullptr,
	                                          NO_SIBLINGS));
	if (!ds) {
		ThrowGDALError(StringUtil::Format("Could not open raster file '%s'", file.real_path));
	}
	if (ds->GetRasterCount() == 0) {
		throw InvalidInputException("Raster file '%s' has no raster bands", file.real_path);
	}
	return ds;
}

unique_ptr<FunctionData> ReadRasterBind(ClientContext &context, TableFunctionBindInput &input,
                                        vector<LogicalType> &return_types, vector<string> &names) {
	const GDALScope scope;
	auto result = make_uniq<ReadRasterBindData>();
	if (input.inputs[0].IsNull()) {
		throw BinderException("ST_ReadRaster: the path cannot be NULL");
	}
	const auto pattern = input.inputs[0].GetValue<string>();

	if (input.inputs.size() == 3) {
		if (input.inputs[1].IsNull() || input.inputs[2].IsNull()) {
			throw BinderException("ST_ReadRaster: the tile size cannot be NULL");
		}
		result->tile_width = input.inputs[1].GetValue<int32_t>();
		result->tile_height = input.inputs[2].GetValue<int32_t>();
		if (result->tile_width <= 0 || result->tile_height <= 0) {
			throw BinderException("ST_ReadRaster: the tile width and height must be positive");
		}
	}

	vector<string> paths;
	if (FileSystem::HasGlob(pattern) && !StringUtil::StartsWith(pattern, "/vsi")) {
		for (auto &file : FileSystem::GetFileSystem(context).GlobFiles(pattern)) {
			paths.push_back(file.path);
		}
	} else {
		paths.push_back(pattern);
	}

	for (const auto &path : paths) {
		RasterFile file;
		file.real_path = path;
		file.gdal_path = GDALFileSystemPath(context, path);
		const auto ds = OpenRasterFile(file);
		file.width = ds->GetRasterXSize();
		file.height = ds->GetRasterYSize();
		const auto tile_width = result->tile_width > 0 ? result->tile_width : file.width;
		const auto tile_height = result->tile_height > 0 ? result->tile_height : file.height;
		file.tiles_x = static_cast<idx_t>((file.width + tile_width - 1) / tile_width);
		file.tiles_y = static_cast<idx_t>((file.height + tile_height - 1) / tile_height);
		file.first_tile = result->tile_count;
		result->tile_count += file.tiles_x * file.tiles_y;
		result->files.push_back(std::move(file));
	}

	names.emplace_back("x");
	return_types.emplace_back(LogicalType::INTEGER);
	names.emplace_back("y");
	return_types.emplace_back(LogicalType::INTEGER);
	names.emplace_back("rast");
	return_types.emplace_back(RasterType());
	return std::move(result);
}

struct ReadRasterGlobalState final : public GlobalTableFunctionState {
	std::atomic<idx_t> next_tile;
	idx_t tile_count;

	explicit ReadRasterGlobalState(idx_t tile_count_p) : next_tile(0), tile_count(tile_count_p) {
	}
	idx_t MaxThreads() const override {
		return MaxValue<idx_t>(1, tile_count / 4);
	}
};

struct ReadRasterLocalState final : public LocalTableFunctionState {
	idx_t file_idx = DConstants::INVALID_INDEX;
	GDALDatasetUniquePtr dataset;
};

unique_ptr<GlobalTableFunctionState> ReadRasterInitGlobal(ClientContext &context, TableFunctionInitInput &input) {
	return make_uniq<ReadRasterGlobalState>(input.bind_data->Cast<ReadRasterBindData>().tile_count);
}

unique_ptr<LocalTableFunctionState> ReadRasterInitLocal(ExecutionContext &context, TableFunctionInitInput &input,
                                                        GlobalTableFunctionState *global_state) {
	return make_uniq<ReadRasterLocalState>();
}

void ReadRasterScan(ClientContext &context, TableFunctionInput &input, DataChunk &output) {
	static constexpr idx_t MAX_CHUNK_BYTES = 32 * 1024 * 1024;
	const GDALScope scope;

	auto &bind_data = input.bind_data->Cast<ReadRasterBindData>();
	auto &gstate = input.global_state->Cast<ReadRasterGlobalState>();
	auto &lstate = input.local_state->Cast<ReadRasterLocalState>();

	const auto xs = FlatVector::GetData<int32_t>(output.data[0]);
	const auto ys = FlatVector::GetData<int32_t>(output.data[1]);
	const auto rasters = FlatVector::GetData<string_t>(output.data[2]);

	idx_t count = 0;
	idx_t bytes = 0;
	while (count < STANDARD_VECTOR_SIZE && bytes < MAX_CHUNK_BYTES) {
		const auto tile = gstate.next_tile++;
		if (tile >= bind_data.tile_count) {
			break;
		}
		idx_t file_idx = 0;
		while (file_idx + 1 < bind_data.files.size() && bind_data.files[file_idx + 1].first_tile <= tile) {
			file_idx++;
		}
		const auto &file = bind_data.files[file_idx];
		if (lstate.file_idx != file_idx) {
			lstate.dataset.reset();
			lstate.dataset = OpenRasterFile(file);
			lstate.file_idx = file_idx;
		}

		const auto tile_width = bind_data.tile_width > 0 ? bind_data.tile_width : file.width;
		const auto tile_height = bind_data.tile_height > 0 ? bind_data.tile_height : file.height;
		const auto tile_x = static_cast<int>((tile - file.first_tile) % file.tiles_x);
		const auto tile_y = static_cast<int>((tile - file.first_tile) / file.tiles_x);
		const auto x0 = tile_x * tile_width;
		const auto y0 = tile_y * tile_height;

		const auto window = CopyWindow(*lstate.dataset, x0, y0, MinValue(tile_width, file.width - x0),
		                               MinValue(tile_height, file.height - y0));
		xs[count] = tile_x;
		ys[count] = tile_y;
		rasters[count] = SerializeRaster(*window, output.data[2]);
		bytes += rasters[count].GetSize();
		count++;
	}
	output.SetCardinality(count);
}

unique_ptr<NodeStatistics> ReadRasterCardinality(ClientContext &context, const FunctionData *bind_data) {
	const auto count = bind_data->Cast<ReadRasterBindData>().tile_count;
	return make_uniq<NodeStatistics>(count, count);
}

void RegisterReadRaster(ExtensionLoader &loader) {
	TableFunctionSet set("ST_ReadRaster");
	for (const auto &arguments : {vector<LogicalType> {LogicalType::VARCHAR},
	                              vector<LogicalType> {LogicalType::VARCHAR, LogicalType::INTEGER,
	                                                   LogicalType::INTEGER}}) {
		TableFunction function("ST_ReadRaster", arguments, ReadRasterScan, ReadRasterBind, ReadRasterInitGlobal,
		                       ReadRasterInitLocal);
		function.cardinality = ReadRasterCardinality;
		set.AddFunction(function);
	}
	loader.RegisterFunction(set);

	InsertionOrderPreservingMap<string> tags;
	tags.insert("ext", "spatial");
	tags.insert("category", "raster");
	FunctionBuilder::AddTableFunctionDocs(loader, "ST_ReadRaster", R"(
		Reads a raster file, or every file matching a glob pattern, and returns its pixels as `RASTER` values.

		Without a tile size the function returns one row per file holding the whole raster. With `tile_width` and `tile_height` (in pixels) each file is cut into tiles of that size, one row per tile; the tiles on the right and bottom edges are smaller when the raster size is not a multiple of the tile size. `x` and `y` are the column and row of the tile in the tile grid, starting at 0, and every tile carries its own georeference.

		Any format of the bundled GDAL raster drivers can be read (see `ST_GDALDrivers()`; GeoTIFF, Cloud Optimized GeoTIFF, VRT and Erdas Imagine `.img`). Files are opened through DuckDB's file system, so remote paths (`https://`, `s3://`, ...) work as they do for `ST_Read`. All bands of a `RASTER` share the pixel type of the first band, colour tables are dropped and side-car files (`.tfw`, `.aux.xml`, `.ovr`) are not read. Tiles are read in parallel and are not returned in order.
	)",
	                                      R"(
		SELECT x, y, ST_Width(rast), ST_Height(rast) FROM ST_ReadRaster('test/data/raster/dem.tif', 16, 16) ORDER BY y, x;
	)",
	                                      tags);
}

//======================================================================================================================
// ST_GDALDrivers
//======================================================================================================================

struct DriversGlobalState final : public GlobalTableFunctionState {
	int current = 0;
};

unique_ptr<FunctionData> DriversBind(ClientContext &context, TableFunctionBindInput &input,
                                     vector<LogicalType> &return_types, vector<string> &names) {
	names.emplace_back("idx");
	return_types.emplace_back(LogicalType::INTEGER);
	names.emplace_back("short_name");
	return_types.emplace_back(LogicalType::VARCHAR);
	names.emplace_back("long_name");
	return_types.emplace_back(LogicalType::VARCHAR);
	names.emplace_back("can_read");
	return_types.emplace_back(LogicalType::BOOLEAN);
	names.emplace_back("can_write");
	return_types.emplace_back(LogicalType::BOOLEAN);
	names.emplace_back("create_options");
	return_types.emplace_back(LogicalType::VARCHAR);
	return make_uniq<TableFunctionData>();
}

unique_ptr<GlobalTableFunctionState> DriversInitGlobal(ClientContext &context, TableFunctionInitInput &input) {
	return make_uniq<DriversGlobalState>();
}

void DriversScan(ClientContext &context, TableFunctionInput &input, DataChunk &output) {
	auto &gstate = input.global_state->Cast<DriversGlobalState>();
	const auto manager = GetGDALDriverManager();

	idx_t count = 0;
	for (; gstate.current < manager->GetDriverCount() && count < STANDARD_VECTOR_SIZE; gstate.current++) {
		const auto driver = manager->GetDriver(gstate.current);
		if (!driver->GetMetadataItem(GDAL_DCAP_RASTER)) {
			continue;
		}
		const auto can_write =
		    driver->GetMetadataItem(GDAL_DCAP_CREATECOPY) != nullptr || driver->GetMetadataItem(GDAL_DCAP_CREATE);
		const auto options = driver->GetMetadataItem(GDAL_DMD_CREATIONOPTIONLIST);
		output.data[0].SetValue(count, Value::INTEGER(gstate.current));
		output.data[1].SetValue(count, Value(driver->GetDescription()));
		output.data[2].SetValue(count, Value(driver->GetMetadataItem(GDAL_DMD_LONGNAME)));
		output.data[3].SetValue(count, Value::BOOLEAN(driver->GetMetadataItem(GDAL_DCAP_OPEN) != nullptr));
		output.data[4].SetValue(count, Value::BOOLEAN(can_write));
		output.data[5].SetValue(count, options ? Value(options) : Value(LogicalType::VARCHAR));
		count++;
	}
	output.SetCardinality(count);
}

void RegisterDrivers(ExtensionLoader &loader) {
	const TableFunction function("ST_GDALDrivers", {}, DriversScan, DriversBind, DriversInitGlobal);
	loader.RegisterFunction(function);

	InsertionOrderPreservingMap<string> tags;
	tags.insert("ext", "spatial");
	tags.insert("category", "raster");
	FunctionBuilder::AddTableFunctionDocs(loader, "ST_GDALDrivers", R"(
		Returns the GDAL raster drivers that are built into the extension: the formats that `ST_ReadRaster` and `ST_FromGDALRaster` can read and that `ST_AsGDALRaster` can write.

		`idx` is the position of the driver in GDAL's driver list, `can_read` and `can_write` tell whether the driver opens and creates files, and `create_options` is the XML description of the creation options that `ST_AsGDALRaster` accepts. The vector drivers are listed by `ST_Drivers()`.
	)",
	                                      R"(
		SELECT short_name, can_read, can_write FROM ST_GDALDrivers() ORDER BY short_name;
	)",
	                                      tags);
}

//======================================================================================================================
// Conversion to and from GDAL formats
//======================================================================================================================

// Drivers that only read the bytes they are given. VRT is left out: it would open the files it references
bool ReadsFromBytes(GDALDriver &driver) {
	return driver.GetMetadataItem(GDAL_DCAP_RASTER) && driver.GetMetadataItem(GDAL_DCAP_OPEN) &&
	       driver.GetMetadataItem(GDAL_DCAP_VIRTUALIO) && !EQUAL(driver.GetDescription(), "VRT") &&
	       !EQUAL(driver.GetDescription(), "MEM");
}

void FromGDALRasterExecute(Call &c) {
	const auto &blob = c.Blob("gdaldata");
	MemFile file(MemFile::NewPath(""));
	const auto handle = VSIFileFromMemBuffer(file.Path().c_str(),
	                                         reinterpret_cast<GByte *>(const_cast<char *>(blob.GetData())),
	                                         static_cast<vsi_l_offset>(blob.GetSize()), FALSE);
	if (!handle) {
		ThrowGDALError("ST_FromGDALRaster: could not read the data");
	}
	VSIFCloseL(handle);

	CPLStringList drivers;
	const auto manager = GetGDALDriverManager();
	for (int i = 0; i < manager->GetDriverCount(); i++) {
		if (ReadsFromBytes(*manager->GetDriver(i))) {
			drivers.AddString(manager->GetDriver(i)->GetDescription());
		}
	}

	CPLErrorReset();
	GDALDatasetUniquePtr source(GDALDataset::Open(file.Path().c_str(),
	                                              GDAL_OF_RASTER | GDAL_OF_READONLY | GDAL_OF_INTERNAL, drivers.List(),
	                                              nullptr, NO_SIBLINGS));
	if (!source || source->GetRasterCount() == 0) {
		ThrowGDALError("ST_FromGDALRaster: the data is not a raster in a supported format (see ST_GDALDrivers())");
	}
	const auto ds = CopyToMem(*source);
	if (c.Has("srid")) {
		SetSRID(*ds, c.Int("srid"));
	}
	c.ReturnRaster(*ds);
}

void AsGDALRasterExecute(Call &c) {
	vector<string> options;
	if (c.Has("options")) {
		options = c.StringList("options");
	}
	if (c.Has("srid")) {
		const auto ds = c.RasterCopy();
		SetSRID(*ds, c.Int("srid"));
		c.ReturnString(SerializeDataset(*ds, c.String("format"), options));
		return;
	}
	c.ReturnString(SerializeDataset(c.Raster(), c.String("format"), options));
}

void AsTIFFExecute(Call &c) {
	vector<string> options;
	if (c.Has("options")) {
		options = c.StringList("options");
	}
	if (c.Has("compression")) {
		// GDAL only warns about a compression it does not know and writes an uncompressed file
		const auto compression = StringUtil::Upper(c.String("compression"));
		const auto driver = GetGDALDriverManager()->GetDriverByName("GTiff");
		const auto creation_options = driver ? driver->GetMetadataItem(GDAL_DMD_CREATIONOPTIONLIST) : nullptr;
		if (!creation_options || !strstr(creation_options, ("<Value>" + compression + "</Value>").c_str())) {
			throw InvalidInputException("ST_AsTIFF: unknown compression '%s'", c.String("compression"));
		}
		options.push_back("COMPRESS=" + compression);
	}
	c.ReturnString(SerializeDataset(c.Raster(), "GTiff", options));
}

} // namespace

//======================================================================================================================
// Registration
//======================================================================================================================

void RegisterRasterIOFunctions(ExtensionLoader &loader) {
	const auto RASTER = RasterType();
	const auto BLOB = LogicalType::BLOB;
	const auto TEXT_LIST = LogicalType::LIST(LogicalType::VARCHAR);

	RegisterReadRaster(loader);
	RegisterDrivers(loader);

	RasterFunction("ST_FromGDALRaster")
	    .AddOptional({Param("gdaldata", BLOB)}, {IntP("srid", true)}, RASTER, FromGDALRasterExecute)
	    .Describe(R"(
		Creates a raster from the bytes of a raster file in any format the bundled GDAL reads from memory (GeoTIFF and Erdas Imagine; see `ST_GDALDrivers()`).

		`srid` overrides the coordinate system of the file with an EPSG code, without reprojecting. VRT files are rejected because they refer to other files: read them with `ST_ReadRaster`. All bands of the result share the pixel type of the first band.
	)",
	              R"(
		SELECT ST_Width(ST_FromGDALRaster(ST_AsGDALRaster(ST_AddBand(ST_MakeEmptyRaster(3, 2, 0, 0, 1), '8BUI'), 'GTiff')));
		----
		3
	)")
	    .Register(loader);

	RasterFunction("ST_AsGDALRaster")
	    .AddOptional({RastP(), TextP("format")}, {Param("options", TEXT_LIST, true), IntP("srid", true)}, BLOB,
	                 AsGDALRasterExecute)
	    .Describe(R"(
		Returns the raster as the bytes of a file in a GDAL raster format.

		`format` is the short name of a driver that can write (see `ST_GDALDrivers()`), `options` is a list of `NAME=VALUE` creation options of that driver, and `srid` overrides the coordinate system written to the file with an EPSG code, without reprojecting. The bundled GDAL has no PNG or JPEG driver. A raster without bands cannot be exported.
	)",
	              R"(
		SELECT octet_length(ST_AsGDALRaster(ST_AddBand(ST_MakeEmptyRaster(3, 2, 0, 0, 1), '8BUI'), 'GTiff', ['COMPRESS=DEFLATE'])) > 0;
		----
		true
	)")
	    .Register(loader);

	RasterFunction("ST_AsTIFF")
	    .Add({RastP()}, BLOB, AsTIFFExecute)
	    .Add({RastP(), TextP("compression")}, BLOB, AsTIFFExecute)
	    .Add({RastP(), Param("options", TEXT_LIST)}, BLOB, AsTIFFExecute)
	    .Describe(R"(
		Returns the raster as the bytes of a GeoTIFF file.

		`compression` is a GeoTIFF compression name: `NONE`, `LZW`, `DEFLATE` or `PACKBITS` (JPEG and ZSTD are not available in the bundled GDAL); `options` is a list of `NAME=VALUE` GeoTIFF creation options. The file carries the coordinate system as regular GeoTIFF keys.

		`RASTER` values are stored uncompressed, because compressing makes writing a raster 10 to 50 times slower. The result of this function is itself a valid `RASTER`: `ST_AsTIFF(rast, 'DEFLATE')::RASTER` is a compressed raster that every function reads transparently, at the price of slower reads.
	)",
	              R"(
		SELECT octet_length(ST_AsTIFF(ST_AddBand(ST_MakeEmptyRaster(3, 2, 0, 0, 1), '8BUI'), 'LZW')) > 0;
		----
		true
	)")
	    .Register(loader);
}

} // namespace raster
} // namespace duckdb
