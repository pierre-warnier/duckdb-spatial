#pragma once

#include "duckdb.hpp"
#include "duckdb/common/types/geometry.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/function/function_set.hpp"
#include "duckdb/parser/parsed_data/create_function_info.hpp"
#include "duckdb/main/extension/extension_loader.hpp"

#include "gdal_priv.h"
#include "ogr_geometry.h"
#include "ogr_spatialref.h"
#include "cpl_conv.h"
#include "cpl_error.h"
#include "cpl_string.h"
#include "cpl_vsi.h"

#include <cmath>
#include <functional>

namespace duckdb {
namespace raster {

//======================================================================================================================
// Type
//======================================================================================================================

// A RASTER value is a BLOB holding a complete GeoTIFF byte stream
LogicalType RasterType();

//======================================================================================================================
// Errors
//======================================================================================================================

// Only call these once a GDAL call has returned: GDAL is not exception-safe, so nothing may throw through its frames
[[noreturn]] void ThrowGDALError(const string &fallback);
void CheckGDAL(CPLErr err, const char *fallback);

//======================================================================================================================
// In-memory files
//======================================================================================================================

class MemFile {
public:
	MemFile() {
	}
	explicit MemFile(string path_p) : path(std::move(path_p)) {
	}
	MemFile(const MemFile &) = delete;
	MemFile &operator=(const MemFile &) = delete;
	MemFile(MemFile &&other) noexcept : path(std::move(other.path)) {
		other.path.clear();
	}
	MemFile &operator=(MemFile &&other) noexcept {
		Reset();
		path = std::move(other.path);
		other.path.clear();
		return *this;
	}
	~MemFile() {
		Reset();
	}

	void Reset() {
		if (!path.empty()) {
			VSIUnlink(path.c_str());
			path.clear();
		}
	}
	const string &Path() const {
		return path;
	}

	// A process-wide unique /vsimem path
	static string NewPath(const char *extension);

private:
	string path;
};

// Removes a whole /vsimem directory, for drivers that write more than one file
class MemDirectory {
public:
	MemDirectory();
	MemDirectory(const MemDirectory &) = delete;
	MemDirectory &operator=(const MemDirectory &) = delete;
	~MemDirectory();

	string File(const string &name) const {
		return path + "/" + name;
	}

private:
	string path;
};

//======================================================================================================================
// Datasets
//======================================================================================================================

// An opened RASTER value. The dataset reads straight from the value's bytes, which must outlive the handle
struct RasterHandle {
	MemFile file;
	GDALDatasetUniquePtr dataset;

	RasterHandle() {
	}
	RasterHandle(RasterHandle &&other) noexcept : file(std::move(other.file)), dataset(std::move(other.dataset)) {
	}
	RasterHandle &operator=(RasterHandle &&other) noexcept {
		Close();
		file = std::move(other.file);
		dataset = std::move(other.dataset);
		return *this;
	}
	~RasterHandle() {
		Close();
	}
	void Close() {
		dataset.reset();
		file.Reset();
	}
};

RasterHandle OpenRaster(const string_t &blob);
RasterHandle OpenRaster(const string &blob);

GDALDatasetUniquePtr CreateMemRaster(int width, int height, int bands, GDALDataType type);
// Same grid and CRS as the source, with new (zero-filled) bands
GDALDatasetUniquePtr CreateMemLike(GDALDataset &src, int bands, GDALDataType type);
GDALDatasetUniquePtr CopyToMem(GDALDataset &src);
// 1-based band numbers, in output order
GDALDatasetUniquePtr SelectBands(GDALDataset &src, const vector<int32_t> &bands);
GDALDatasetUniquePtr CopyWindow(GDALDataset &src, int x, int y, int width, int height);
void CopyGeoreference(GDALDataset &src, GDALDataset &dst);
void AddBand(GDALDataset &ds, GDALDataType type);
// Copies the pixels and the NODATA value into a band of an in-memory raster of the same size
void CopyBand(GDALRasterBand &src, GDALRasterBand &dst);

string SerializeRaster(GDALDataset &ds);
Value RasterValue(GDALDataset &ds);
string_t SerializeRaster(GDALDataset &ds, Vector &result);
// Writes the dataset with any creation-capable driver and returns the file's bytes
string SerializeDataset(GDALDataset &ds, const string &driver, const vector<string> &options);

//======================================================================================================================
// Bands and pixels
//======================================================================================================================

GDALRasterBand &GetBand(GDALDataset &ds, int32_t band);
bool GetNoData(GDALRasterBand &band, double &nodata);
void SetNoData(GDALRasterBand &band, bool has_nodata, double nodata);

inline bool IsNoData(double value, bool has_nodata, double nodata) {
	return has_nodata && (value == nodata || (std::isnan(value) && std::isnan(nodata)));
}

void ReadPixels(GDALRasterBand &band, int x, int y, int width, int height, double *out);
void WritePixels(GDALRasterBand &band, int x, int y, int width, int height, const double *in);
vector<double> ReadBand(GDALRasterBand &band);
void WriteBand(GDALRasterBand &band, const vector<double> &values);

// Pixel values of a band, with the nodata pixels flagged
struct BandValues {
	int width;
	int height;
	bool has_nodata;
	double nodata;
	vector<double> values;

	BandValues(GDALRasterBand &band, bool exclude_nodata);
	bool IsValid(idx_t idx) const {
		return !IsNoData(values[idx], has_nodata, nodata);
	}
};

GDALDataType ParsePixelType(const string &name);
// The value a band of this type stores for `value`
double ClampToPixelType(double value, GDALDataType type);
string PixelTypeName(GDALDataType type);
double MinPossibleValue(GDALDataType type);

//======================================================================================================================
// Georeferencing
//======================================================================================================================

struct GeoTransform {
	// GDAL order: upperleftx, scalex, skewx, upperlefty, skewy, scaley
	double c[6];

	GeoTransform();
	explicit GeoTransform(GDALDataset &ds);

	void Apply(GDALDataset &ds) const;
	// 0-based, fractional pixel coordinates
	void ToWorld(double col, double row, double &x, double &y) const;
	void ToPixel(double x, double y, double &col, double &row) const;
};

// 0-based integer cell containing a fractional pixel coordinate, tolerant of rounding noise on cell edges
int64_t PixelFloor(double value);

struct Grid {
	GeoTransform gt;
	int width;
	int height;
};

OGREnvelope RasterEnvelope(GDALDataset &ds);
void CheckGridSize(double width, double height);
// The smallest grid with the given pixel vectors that covers the envelope and whose pixel corners lie on the lattice
// through the anchor point
Grid CoverEnvelope(const OGREnvelope &envelope, double scalex, double scaley, double skewx, double skewy,
                   double anchor_x, double anchor_y);

// The coordinate system of a raster is text: AUTHORITY:CODE when it has an authority code, WKT otherwise
string SpatialRefToText(const OGRSpatialReference &srs);
string GetCRS(GDALDataset &ds);
int32_t GetSRID(GDALDataset &ds);
// Returns false if the raster has no coordinate system
bool GetSpatialRef(GDALDataset &ds, OGRSpatialReference &srs);
// Stores the text as it is
void SetCRSText(GDALDataset &ds, const string &crs);
// Validates and normalises user input
void SetCRS(GDALDataset &ds, const string &crs);
void SetSRID(GDALDataset &ds, int32_t srid);
bool SameAlignment(GDALDataset &a, GDALDataset &b, string &reason);

//======================================================================================================================
// Geometry
//======================================================================================================================

OGRGeometryUniquePtr GeometryFromWKB(const string_t &wkb);
string GeometryToWKB(const OGRGeometry &geom);
Value GeometryValue(const OGRGeometry &geom);
OGRGeometryUniquePtr MakePolygon(const double *xs, const double *ys, idx_t count);
// The polygon covering pixels [x0, x1) x [y0, y1), 0-based
OGRGeometryUniquePtr PixelPolygon(const GeoTransform &gt, double x0, double y0, double x1, double y1);

// Burns a value into every pixel of the band the geometry covers (pixel centre rule, or any touched pixel)
void BurnGeometry(GDALDataset &ds, int32_t band, const OGRGeometry &geom, double value, bool all_touched);

//======================================================================================================================
// Row-wise scalar function support
//======================================================================================================================

struct Param {
	const char *name;
	LogicalType type;
	// A NULL in a non-nullable parameter makes the result NULL without calling the function
	bool nullable;

	Param(const char *name_p, LogicalType type_p, bool nullable_p = false)
	    : name(name_p), type(std::move(type_p)), nullable(nullable_p) {
	}
};

inline Param RastP(const char *name = "rast") {
	return Param(name, RasterType());
}
inline Param IntP(const char *name, bool nullable = false) {
	return Param(name, LogicalType::INTEGER, nullable);
}
inline Param DblP(const char *name, bool nullable = false) {
	return Param(name, LogicalType::DOUBLE, nullable);
}
inline Param BoolP(const char *name) {
	return Param(name, LogicalType::BOOLEAN);
}
inline Param TextP(const char *name, bool nullable = false) {
	return Param(name, LogicalType::VARCHAR, nullable);
}
inline Param GeomP(const char *name = "geom") {
	return Param(name, LogicalType::GEOMETRY());
}

class Call {
public:
	Call(const vector<Param> &params, DataChunk &args, ExpressionState &state, Vector &result);

	// Whether this overload has the parameter at all
	bool Declares(const char *name) const;
	// Whether the parameter is declared and not NULL in the current row
	bool Has(const char *name) const;

	int32_t Int(const char *name) const;
	int32_t Int(const char *name, int32_t fallback) const;
	double Double(const char *name) const;
	double Double(const char *name, double fallback) const;
	bool Bool(const char *name) const;
	bool Bool(const char *name, bool fallback) const;
	string String(const char *name) const;
	string String(const char *name, const string &fallback) const;
	const string_t &Blob(const char *name) const;
	Value GetValue(const char *name) const;
	vector<double> DoubleList(const char *name) const;
	vector<int32_t> IntList(const char *name) const;
	vector<string> StringList(const char *name) const;

	// Read-only dataset over the argument; stays open while consecutive rows carry the same value
	GDALDataset &Raster(const char *name = "rast");
	GDALDatasetUniquePtr RasterCopy(const char *name = "rast");
	GDALRasterBand &Band(const char *raster_name = "rast", const char *band_name = "band");
	OGRGeometryUniquePtr Geometry(const char *name = "geom") const;

	template <class T>
	void Return(T value) {
		FlatVector::GetData<T>(result)[row] = value;
		returned = true;
	}
	void ReturnString(const string &value);
	void ReturnRaster(GDALDataset &ds);
	void ReturnGeometry(const OGRGeometry &geom);
	void ReturnValue(const Value &value);

	Vector &Result() {
		returned = true;
		return result;
	}
	idx_t Row() const {
		return row;
	}
	ClientContext &Context() const;

private:
	friend void ExecuteCall(const vector<Param> &params, const std::function<void(Call &)> &function, DataChunk &args,
	                        ExpressionState &state, Vector &result);

	struct Slot {
		const char *data = nullptr;
		idx_t size = 0;
		RasterHandle handle;
	};

	idx_t Find(const char *name) const;
	idx_t Require(const char *name) const;
	bool IsNull(idx_t col) const;
	template <class T>
	const T &Get(idx_t col) const {
		return UnifiedVectorFormat::GetData<T>(formats[col])[formats[col].sel->get_index(row)];
	}

	const vector<Param> &params;
	DataChunk &args;
	ExpressionState &state;
	Vector &result;
	vector<UnifiedVectorFormat> formats;
	vector<Slot> slots;
	idx_t row;
	bool returned;
};

typedef void (*raster_function_t)(Call &call);

void ExecuteCall(const vector<Param> &params, const std::function<void(Call &)> &function, DataChunk &args,
                 ExpressionState &state, Vector &result);

// Gives the bind callback of a custom function the parameters of the overload that was chosen
struct ParamsInfo final : public ScalarFunctionInfo {
	explicit ParamsInfo(shared_ptr<vector<Param>> params_p) : params(std::move(params_p)) {
	}
	shared_ptr<vector<Param>> params;
};

class RasterFunction {
public:
	explicit RasterFunction(const char *name);

	RasterFunction &Add(vector<Param> params, const LogicalType &return_type, raster_function_t function);
	// One overload per prefix of the optional parameters
	RasterFunction &AddOptional(const vector<Param> &required, const vector<Param> &optional,
	                            const LogicalType &return_type, raster_function_t function);
	RasterFunction &Describe(const char *description, const char *example);
	// For functions that need bind data or a local state: every overload runs `function`, which finds its parameters
	// in the ParamsInfo of the bound function
	RasterFunction &Custom(scalar_function_t function, bind_scalar_function_t bind, init_local_state_t init);
	void Register(ExtensionLoader &loader);

private:
	struct Variant {
		shared_ptr<vector<Param>> params;
		LogicalType return_type;
		raster_function_t function;
	};

	const char *name;
	const char *description;
	const char *example;
	vector<Variant> variants;
	scalar_function_t custom_function;
	bind_scalar_function_t custom_bind;
	init_local_state_t custom_init;
};

// Keeps the tags of a function that already exists when overloads are added to it
InsertionOrderPreservingMap<string> FunctionTags(ExtensionLoader &loader, CatalogType type, const char *name);

// Registers aggregate overloads. DuckDB cannot add overloads to an existing aggregate, so an aggregate of the same
// name is replaced by one that has both its overloads and the new ones
void RegisterAggregate(ExtensionLoader &loader, AggregateFunctionSet set, vector<FunctionDescription> descriptions);

//======================================================================================================================
// Function families
//======================================================================================================================

void RegisterRasterType(ExtensionLoader &loader);
void RegisterRasterBasicFunctions(ExtensionLoader &loader);
void RegisterRasterIOFunctions(ExtensionLoader &loader);
void RegisterRasterProcessingFunctions(ExtensionLoader &loader);
void RegisterRasterStatisticsFunctions(ExtensionLoader &loader);
void RegisterRasterMapAlgebraFunctions(ExtensionLoader &loader);
void RegisterRasterVectorFunctions(ExtensionLoader &loader);

} // namespace raster
} // namespace duckdb
