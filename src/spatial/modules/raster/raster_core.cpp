#include "spatial/modules/raster/raster_core.hpp"
#include "spatial/util/function_builder.hpp"

#include "duckdb/catalog/catalog.hpp"
#include "duckdb/catalog/catalog_entry/function_entry.hpp"
#include "duckdb/catalog/catalog_entry/schema_catalog_entry.hpp"
#include "duckdb/function/cast/default_casts.hpp"
#include "duckdb/common/operator/cast_operators.hpp"
#include "duckdb/parser/parsed_data/create_scalar_function_info.hpp"

#include "gdal_alg.h"
#include "memdataset.h"

#include <atomic>
#include <cfloat>
#include <climits>

namespace duckdb {
namespace raster {

//======================================================================================================================
// Type
//======================================================================================================================

LogicalType RasterType() {
	auto type = LogicalType(LogicalTypeId::BLOB);
	type.SetAlias("RASTER");
	return type;
}

//======================================================================================================================
// Errors
//======================================================================================================================

void ThrowGDALError(const string &fallback) {
	string msg = fallback;
	if (CPLGetLastErrorType() >= CE_Failure) {
		const string detail = CPLGetLastErrorMsg();
		if (!detail.empty()) {
			msg += ": " + detail;
		}
	}
	CPLErrorReset();
	throw InvalidInputException(msg);
}

void CheckGDAL(CPLErr err, const char *fallback) {
	if (err >= CE_Failure) {
		ThrowGDALError(fallback);
	}
}

//======================================================================================================================
// In-memory files
//======================================================================================================================

static string NextMemName() {
	static std::atomic<uint64_t> counter(0);
	return "/vsimem/duckdb_raster_" + std::to_string(counter++);
}

string MemFile::NewPath(const char *extension) {
	return NextMemName() + extension;
}

MemDirectory::MemDirectory() : path(NextMemName()) {
	VSIMkdir(path.c_str(), 0755);
}

MemDirectory::~MemDirectory() {
	VSIRmdirRecursive(path.c_str());
}

//======================================================================================================================
// Datasets
//======================================================================================================================

// GeoTIFF has a single nodata value and at least one band. Rasters that do not fit are stored with these markers
static const char *const NUMBANDS_ITEM = "RASTER_NUMBANDS";
// The coordinate system is stored as text. Translating it to and from GeoTIFF keys goes through the PROJ database and
// costs far more than everything else that happens to a small raster
static const char *const CRS_ITEM = "RASTER_CRS";
static const char *const NODATA_ITEM = "RASTER_NODATA";
static const char *const NODATA_NONE = "none";

static GDALDriver &GetDriver(const char *name) {
	const auto driver = GetGDALDriverManager()->GetDriverByName(name);
	if (!driver) {
		throw InvalidInputException("GDAL driver '%s' is not available", name);
	}
	return *driver;
}

GDALDatasetUniquePtr CreateMemRaster(int width, int height, int bands, GDALDataType type) {
	if (width <= 0 || height <= 0) {
		throw InvalidInputException("Raster width and height must be positive, got %d x %d", width, height);
	}
	CPLErrorReset();
	// Straight to the driver: GDALDriver::Create() validates options and probes for an existing dataset on every call
	GDALDatasetUniquePtr result(MEMDataset::Create("", width, height, bands, type, nullptr));
	if (!result) {
		ThrowGDALError(StringUtil::Format("Could not allocate a %d x %d raster with %d band(s)", width, height, bands));
	}
	return result;
}

void CopyGeoreference(GDALDataset &src, GDALDataset &dst) {
	double gt[6];
	if (src.GetGeoTransform(gt) == CE_None) {
		CheckGDAL(dst.SetGeoTransform(gt), "Could not set the georeference");
	}
	SetCRSText(dst, GetCRS(src));
}

GDALDatasetUniquePtr CreateMemLike(GDALDataset &src, int bands, GDALDataType type) {
	auto result = CreateMemRaster(src.GetRasterXSize(), src.GetRasterYSize(), bands, type);
	CopyGeoreference(src, *result);
	return result;
}

void AddBand(GDALDataset &ds, GDALDataType type) {
	if (ds.GetRasterCount() > 0 && ds.GetRasterBand(1)->GetRasterDataType() != type) {
		throw InvalidInputException("All bands of a RASTER share one pixel type: the raster has %s bands, cannot add "
		                            "a %s band",
		                            PixelTypeName(ds.GetRasterBand(1)->GetRasterDataType()), PixelTypeName(type));
	}
	CPLErrorReset();
	CheckGDAL(ds.AddBand(type, nullptr), "Could not add a band");
}

static void CopyBandPixels(GDALRasterBand &src, int x, int y, GDALRasterBand &dst) {
	const auto width = dst.GetXSize();
	const auto height = dst.GetYSize();
	const auto data = static_cast<MEMRasterBand &>(dst).GetData();
	CPLErrorReset();
	CheckGDAL(src.RasterIO(GF_Read, x, y, width, height, data, width, height, dst.GetRasterDataType(), 0, 0, nullptr),
	          "Could not read raster pixels");
}

static void CopyBandNoData(GDALRasterBand &src, GDALRasterBand &dst) {
	double nodata;
	if (GetNoData(src, nodata)) {
		dst.SetNoDataValue(nodata);
	}
}

void CopyBand(GDALRasterBand &src, GDALRasterBand &dst) {
	if (src.GetXSize() != dst.GetXSize() || src.GetYSize() != dst.GetYSize()) {
		throw InvalidInputException("The rasters must have the same width and height");
	}
	CopyBandPixels(src, 0, 0, dst);
	CopyBandNoData(src, dst);
}

GDALDatasetUniquePtr CopyWindow(GDALDataset &src, int x, int y, int width, int height) {
	const auto band_count = src.GetRasterCount();
	const auto type = band_count > 0 ? src.GetRasterBand(1)->GetRasterDataType() : GDT_Byte;
	auto result = CreateMemRaster(width, height, band_count, type);

	GeoTransform gt(src);
	gt.ToWorld(x, y, gt.c[0], gt.c[3]);
	gt.Apply(*result);
	SetCRSText(*result, GetCRS(src));

	for (int i = 1; i <= band_count; i++) {
		auto &src_band = *src.GetRasterBand(i);
		auto &dst_band = *result->GetRasterBand(i);
		CopyBandPixels(src_band, x, y, dst_band);
		CopyBandNoData(src_band, dst_band);
	}
	return result;
}

GDALDatasetUniquePtr CopyToMem(GDALDataset &src) {
	const auto band_count = src.GetRasterCount();
	const auto type = band_count > 0 ? src.GetRasterBand(1)->GetRasterDataType() : GDT_Byte;
	auto result = CreateMemLike(src, band_count, type);
	for (int i = 1; i <= band_count; i++) {
		auto &src_band = *src.GetRasterBand(i);
		auto &dst_band = *result->GetRasterBand(i);
		CopyBandPixels(src_band, 0, 0, dst_band);
		CopyBandNoData(src_band, dst_band);
	}
	return result;
}

GDALDatasetUniquePtr SelectBands(GDALDataset &src, const vector<int32_t> &bands) {
	for (const auto band : bands) {
		GetBand(src, band);
	}
	const auto type = bands.empty() ? GDT_Byte : src.GetRasterBand(bands[0])->GetRasterDataType();
	auto result = CreateMemLike(src, static_cast<int>(bands.size()), type);
	for (idx_t i = 0; i < bands.size(); i++) {
		auto &src_band = *src.GetRasterBand(bands[i]);
		auto &dst_band = *result->GetRasterBand(static_cast<int>(i + 1));
		CopyBandPixels(src_band, 0, 0, dst_band);
		CopyBandNoData(src_band, dst_band);
	}
	return result;
}

// Restores what the markers describe, as a plain in-memory dataset
static void Normalize(RasterHandle &handle) {
	auto &ds = *handle.dataset;
	const auto band_count_item = ds.GetMetadataItem(NUMBANDS_ITEM);
	if (band_count_item && EQUAL(band_count_item, "0")) {
		auto mem = CreateMemLike(ds, 0, GDT_Byte);
		handle.dataset = std::move(mem);
		handle.file.Reset();
		return;
	}
	if (!ds.GetRasterBand(1)->GetMetadataItem(NODATA_ITEM)) {
		return;
	}
	auto mem = CopyToMem(ds);
	for (int i = 1; i <= ds.GetRasterCount(); i++) {
		const auto item = ds.GetRasterBand(i)->GetMetadataItem(NODATA_ITEM);
		auto &band = *mem->GetRasterBand(i);
		if (!item || EQUAL(item, NODATA_NONE)) {
			band.DeleteNoDataValue();
		} else {
			band.SetNoDataValue(CPLAtof(item));
		}
	}
	handle.dataset = std::move(mem);
	handle.file.Reset();
}

static bool TryOpenRaster(const char *data, idx_t size, RasterHandle &result) {
	if (size < 8) {
		return false;
	}
	auto path = MemFile::NewPath(".tif");
	const auto file = VSIFileFromMemBuffer(path.c_str(), reinterpret_cast<GByte *>(const_cast<char *>(data)),
	                                       static_cast<vsi_l_offset>(size), FALSE);
	if (!file) {
		CPLErrorReset();
		return false;
	}
	VSIFCloseL(file);
	result.file = MemFile(std::move(path));

	static const char *const DRIVERS[] = {"GTiff", nullptr};
	// An empty, non-null sibling list keeps GDAL from scanning /vsimem for side-car files
	static const char *const NO_SIBLINGS[] = {nullptr};
	CPLErrorReset();
	result.dataset.reset(GDALDataset::Open(result.file.Path().c_str(),
	                                       GDAL_OF_RASTER | GDAL_OF_READONLY | GDAL_OF_INTERNAL, DRIVERS, nullptr,
	                                       NO_SIBLINGS));
	if (!result.dataset || result.dataset->GetRasterCount() == 0) {
		CPLErrorReset();
		result.Close();
		return false;
	}
	CPLErrorReset();
	Normalize(result);
	return true;
}

RasterHandle OpenRaster(const string_t &blob) {
	RasterHandle result;
	if (!TryOpenRaster(blob.GetData(), blob.GetSize(), result)) {
		throw InvalidInputException("Invalid RASTER value: not a GeoTIFF byte stream (use ST_FromGDALRaster to "
		                            "convert a raster file)");
	}
	return result;
}

RasterHandle OpenRaster(const string &blob) {
	RasterHandle result;
	if (!TryOpenRaster(blob.data(), blob.size(), result)) {
		throw InvalidInputException("Invalid RASTER value: not a GeoTIFF byte stream (use ST_FromGDALRaster to "
		                            "convert a raster file)");
	}
	return result;
}

static bool HasUniformNoData(GDALDataset &ds) {
	double first = 0;
	const auto has_first = GetNoData(*ds.GetRasterBand(1), first);
	for (int i = 2; i <= ds.GetRasterCount(); i++) {
		double other = 0;
		const auto has_other = GetNoData(*ds.GetRasterBand(i), other);
		if (has_other != has_first) {
			return false;
		}
		if (has_first && other != first && !(std::isnan(other) && std::isnan(first))) {
			return false;
		}
	}
	return true;
}

static void WriteGTiff(GDALDataset &ds, const string &path, CPLStringList &options) {
	static GDALDriver &driver = GetDriver("GTiff");
	CPLErrorReset();

	// Datasets that come out of GDAL's own algorithms may carry a real coordinate system
	const auto srs = ds.GetSpatialRef();
	if (srs && !srs->IsEmpty()) {
		if (!ds.GetMetadataItem(CRS_ITEM)) {
			SetCRSText(ds, SpatialRefToText(*srs));
		}
		ds.SetSpatialRef(nullptr);
		CPLErrorReset();
	}

	if (ds.GetRasterCount() == 0) {
		CPLStringList sparse;
		sparse.SetNameValue("SPARSE_OK", "TRUE");
		GDALDatasetUniquePtr out(
		    driver.Create(path.c_str(), ds.GetRasterXSize(), ds.GetRasterYSize(), 1, GDT_Byte, sparse.List()));
		if (!out) {
			ThrowGDALError("Could not serialize raster");
		}
		double gt[6];
		if (ds.GetGeoTransform(gt) == CE_None) {
			out->SetGeoTransform(gt);
		}
		out->SetMetadataItem(CRS_ITEM, ds.GetMetadataItem(CRS_ITEM));
		out->SetMetadataItem(NUMBANDS_ITEM, "0");
		out.reset();
		if (CPLGetLastErrorType() >= CE_Failure) {
			ThrowGDALError("Could not serialize raster");
		}
		return;
	}

	const auto uniform = HasUniformNoData(ds);
	if (!uniform) {
		for (int i = 1; i <= ds.GetRasterCount(); i++) {
			auto &band = *ds.GetRasterBand(i);
			double nodata;
			if (GetNoData(band, nodata)) {
				band.SetMetadataItem(NODATA_ITEM, CPLSPrintf("%.17g", nodata));
			} else {
				band.SetMetadataItem(NODATA_ITEM, NODATA_NONE);
			}
		}
	}
	GDALDatasetUniquePtr out(driver.pfnCreateCopy(path.c_str(), &ds, FALSE, options.List(), nullptr, nullptr));
	if (!uniform) {
		for (int i = 1; i <= ds.GetRasterCount(); i++) {
			ds.GetRasterBand(i)->SetMetadataItem(NODATA_ITEM, nullptr);
		}
	}
	if (!out) {
		ThrowGDALError("Could not serialize raster");
	}
	out.reset();
	if (CPLGetLastErrorType() >= CE_Failure) {
		ThrowGDALError("Could not serialize raster");
	}
}

struct CPLBufferDeleter {
	void operator()(GByte *ptr) const {
		CPLFree(ptr);
	}
};

// Takes the bytes of a /vsimem file, which removes the file
static std::unique_ptr<GByte, CPLBufferDeleter> StealMemFile(MemFile &file, vsi_l_offset &size) {
	std::unique_ptr<GByte, CPLBufferDeleter> buffer(VSIGetMemFileBuffer(file.Path().c_str(), &size, TRUE));
	if (!buffer) {
		ThrowGDALError("Could not read back the serialized raster");
	}
	return buffer;
}

static std::unique_ptr<GByte, CPLBufferDeleter> SerializeToBuffer(GDALDataset &ds, vsi_l_offset &size) {
	MemFile file(MemFile::NewPath(".tif"));
	CPLStringList options;
	options.SetNameValue("INTERLEAVE", "BAND");
	WriteGTiff(ds, file.Path(), options);
	return StealMemFile(file, size);
}

string SerializeRaster(GDALDataset &ds) {
	vsi_l_offset size = 0;
	const auto buffer = SerializeToBuffer(ds, size);
	return string(const_char_ptr_cast(buffer.get()), static_cast<size_t>(size));
}

Value RasterValue(GDALDataset &ds) {
	auto value = Value::BLOB_RAW(SerializeRaster(ds));
	value.Reinterpret(RasterType());
	return value;
}

string_t SerializeRaster(GDALDataset &ds, Vector &result) {
	vsi_l_offset size = 0;
	const auto buffer = SerializeToBuffer(ds, size);
	return StringVector::AddStringOrBlob(result, const_char_ptr_cast(buffer.get()), static_cast<idx_t>(size));
}

string SerializeDataset(GDALDataset &source, const string &driver_name, const vector<string> &option_list) {
	const auto driver = GetGDALDriverManager()->GetDriverByName(driver_name.c_str());
	if (!driver || !driver->GetMetadataItem(GDAL_DCAP_RASTER)) {
		throw InvalidInputException("Unknown GDAL raster driver '%s', see ST_GDALDrivers()", driver_name);
	}
	if (!driver->GetMetadataItem(GDAL_DCAP_CREATECOPY) && !driver->GetMetadataItem(GDAL_DCAP_CREATE)) {
		throw InvalidInputException("GDAL driver '%s' cannot write rasters", driver_name);
	}
	if (source.GetRasterCount() == 0) {
		throw InvalidInputException("Cannot export a raster without bands");
	}
	CPLStringList options;
	for (const auto &option : option_list) {
		options.AddString(option.c_str());
	}

	// Files that leave the database get a regular coordinate system instead of the internal text form
	const auto ds = CopyToMem(source);
	OGRSpatialReference srs;
	if (GetSpatialRef(*ds, srs)) {
		CheckGDAL(ds->SetSpatialRef(&srs), "Could not set the coordinate system");
	}
	ds->SetMetadataItem(CRS_ITEM, nullptr);

	MemDirectory directory;
	const auto path = directory.File("raster");
	CPLErrorReset();
	GDALDatasetUniquePtr out(driver->CreateCopy(path.c_str(), ds.get(), FALSE, options.List(), nullptr, nullptr));
	if (!out) {
		ThrowGDALError(StringUtil::Format("Could not export the raster with driver '%s'", driver_name));
	}
	out.reset();
	if (CPLGetLastErrorType() >= CE_Failure) {
		ThrowGDALError(StringUtil::Format("Could not export the raster with driver '%s'", driver_name));
	}

	vsi_l_offset size = 0;
	const auto data = VSIGetMemFileBuffer(path.c_str(), &size, FALSE);
	if (!data) {
		throw InvalidInputException("GDAL driver '%s' did not produce a file", driver_name);
	}
	return string(const_char_ptr_cast(data), static_cast<size_t>(size));
}

//======================================================================================================================
// Bands and pixels
//======================================================================================================================

GDALRasterBand &GetBand(GDALDataset &ds, int32_t band) {
	const auto count = ds.GetRasterCount();
	if (band < 1 || band > count) {
		throw InvalidInputException("Band %d is out of range: the raster has %d band(s), numbered from 1", band, count);
	}
	return *ds.GetRasterBand(band);
}

bool GetNoData(GDALRasterBand &band, double &nodata) {
	int has_nodata = 0;
	nodata = band.GetNoDataValue(&has_nodata);
	return has_nodata != 0;
}

void SetNoData(GDALRasterBand &band, bool has_nodata, double nodata) {
	CPLErrorReset();
	if (has_nodata) {
		CheckGDAL(band.SetNoDataValue(nodata), "Could not set the NODATA value");
	} else {
		band.DeleteNoDataValue();
		CPLErrorReset();
	}
}

void ReadPixels(GDALRasterBand &band, int x, int y, int width, int height, double *out) {
	CPLErrorReset();
	CheckGDAL(band.RasterIO(GF_Read, x, y, width, height, out, width, height, GDT_Float64, 0, 0, nullptr),
	          "Could not read raster pixels");
}

void WritePixels(GDALRasterBand &band, int x, int y, int width, int height, const double *in) {
	CPLErrorReset();
	CheckGDAL(band.RasterIO(GF_Write, x, y, width, height, const_cast<double *>(in), width, height, GDT_Float64, 0, 0,
	                        nullptr),
	          "Could not write raster pixels");
}

vector<double> ReadBand(GDALRasterBand &band) {
	vector<double> values(static_cast<idx_t>(band.GetXSize()) * static_cast<idx_t>(band.GetYSize()));
	ReadPixels(band, 0, 0, band.GetXSize(), band.GetYSize(), values.data());
	return values;
}

void WriteBand(GDALRasterBand &band, const vector<double> &values) {
	WritePixels(band, 0, 0, band.GetXSize(), band.GetYSize(), values.data());
}

BandValues::BandValues(GDALRasterBand &band, bool exclude_nodata)
    : width(band.GetXSize()), height(band.GetYSize()), has_nodata(false), nodata(0), values(ReadBand(band)) {
	if (exclude_nodata) {
		has_nodata = GetNoData(band, nodata);
	}
}

GDALDataType ParsePixelType(const string &name) {
	const auto upper = StringUtil::Upper(name);
	if (upper == "8BUI" || upper == "1BB" || upper == "2BUI" || upper == "4BUI") {
		return GDT_Byte;
	}
	if (upper == "8BSI") {
		return GDT_Int8;
	}
	if (upper == "16BUI") {
		return GDT_UInt16;
	}
	if (upper == "16BSI") {
		return GDT_Int16;
	}
	if (upper == "32BUI") {
		return GDT_UInt32;
	}
	if (upper == "32BSI") {
		return GDT_Int32;
	}
	if (upper == "32BF") {
		return GDT_Float32;
	}
	if (upper == "64BF") {
		return GDT_Float64;
	}
	throw InvalidInputException("Unknown pixel type '%s', expected one of 8BUI, 8BSI, 16BUI, 16BSI, 32BUI, 32BSI, "
	                            "32BF, 64BF (1BB, 2BUI and 4BUI are stored as 8BUI)",
	                            name);
}

string PixelTypeName(GDALDataType type) {
	switch (type) {
	case GDT_Byte:
		return "8BUI";
	case GDT_Int8:
		return "8BSI";
	case GDT_UInt16:
		return "16BUI";
	case GDT_Int16:
		return "16BSI";
	case GDT_UInt32:
		return "32BUI";
	case GDT_Int32:
		return "32BSI";
	case GDT_Float32:
		return "32BF";
	case GDT_Float64:
		return "64BF";
	default:
		return GDALGetDataTypeName(type);
	}
}

double MinPossibleValue(GDALDataType type) {
	switch (type) {
	case GDT_Int8:
		return -128;
	case GDT_Int16:
		return -32768;
	case GDT_Int32:
		return INT_MIN;
	case GDT_Float32:
		return -FLT_MAX;
	case GDT_Float64:
		return -DBL_MAX;
	default:
		return 0;
	}
}

//======================================================================================================================
// Georeferencing
//======================================================================================================================

GeoTransform::GeoTransform() : c {0, 1, 0, 0, 0, 1} {
}

GeoTransform::GeoTransform(GDALDataset &ds) : c {0, 1, 0, 0, 0, 1} {
	double gt[6];
	if (ds.GetGeoTransform(gt) == CE_None) {
		memcpy(c, gt, sizeof(c));
	}
}

void GeoTransform::Apply(GDALDataset &ds) const {
	CPLErrorReset();
	CheckGDAL(ds.SetGeoTransform(const_cast<double *>(c)), "Could not set the georeference");
}

void GeoTransform::ToWorld(double col, double row, double &x, double &y) const {
	const auto wx = c[0] + col * c[1] + row * c[2];
	const auto wy = c[3] + col * c[4] + row * c[5];
	x = wx;
	y = wy;
}

void GeoTransform::ToPixel(double x, double y, double &col, double &row) const {
	const auto det = c[1] * c[5] - c[2] * c[4];
	if (det == 0 || !std::isfinite(det)) {
		throw InvalidInputException("The raster georeference is not invertible (scale and skew are degenerate)");
	}
	const auto dx = x - c[0];
	const auto dy = y - c[3];
	col = (dx * c[5] - dy * c[2]) / det;
	row = (dy * c[1] - dx * c[4]) / det;
}

int64_t PixelFloor(double value) {
	const auto rounded = std::round(value);
	if (std::fabs(value - rounded) < 1e-7) {
		return static_cast<int64_t>(rounded);
	}
	return static_cast<int64_t>(std::floor(value));
}

string SpatialRefToText(const OGRSpatialReference &srs) {
	const auto authority = srs.GetAuthorityName(nullptr);
	const auto code = srs.GetAuthorityCode(nullptr);
	if (authority && code) {
		return string(authority) + ":" + code;
	}
	char *wkt = nullptr;
	const char *const options[] = {"FORMAT=WKT2_2019", nullptr};
	srs.exportToWkt(&wkt, options);
	const string result = wkt ? wkt : "";
	CPLFree(wkt);
	CPLErrorReset();
	return result;
}

string GetCRS(GDALDataset &ds) {
	const auto item = ds.GetMetadataItem(CRS_ITEM);
	if (item) {
		return item;
	}
	// A GeoTIFF that was not written by this module
	const auto srs = ds.GetSpatialRef();
	if (!srs || srs->IsEmpty()) {
		return string();
	}
	return SpatialRefToText(*srs);
}

void SetCRSText(GDALDataset &ds, const string &crs) {
	ds.SetMetadataItem(CRS_ITEM, crs.empty() ? nullptr : crs.c_str());
}

int32_t GetSRID(GDALDataset &ds) {
	const auto crs = GetCRS(ds);
	if (crs.size() > 5 && StringUtil::CIEquals(crs.substr(0, 5), "EPSG:")) {
		return atoi(crs.c_str() + 5);
	}
	return 0;
}

static void ParseSpatialRef(const string &crs, OGRSpatialReference &srs) {
	CPLErrorReset();
	if (srs.SetFromUserInput(crs.c_str()) != OGRERR_NONE) {
		ThrowGDALError(StringUtil::Format("Could not parse the coordinate system '%s'", crs));
	}
	srs.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
	CPLErrorReset();
}

bool GetSpatialRef(GDALDataset &ds, OGRSpatialReference &srs) {
	const auto crs = GetCRS(ds);
	if (crs.empty()) {
		return false;
	}
	ParseSpatialRef(crs, srs);
	return true;
}

void SetCRS(GDALDataset &ds, const string &crs) {
	if (crs.empty()) {
		SetCRSText(ds, crs);
		return;
	}
	// The same coordinate system is usually set on many rasters in a row
	static thread_local string last_input;
	static thread_local string last_text;
	if (last_input != crs || last_text.empty()) {
		OGRSpatialReference srs;
		ParseSpatialRef(crs, srs);
		last_text = SpatialRefToText(srs);
		last_input = crs;
	}
	SetCRSText(ds, last_text);
}

void SetSRID(GDALDataset &ds, int32_t srid) {
	if (srid <= 0) {
		SetCRSText(ds, string());
		return;
	}
	SetCRS(ds, "EPSG:" + std::to_string(srid));
}

static bool SameCRS(GDALDataset &a, GDALDataset &b) {
	const auto crs_a = GetCRS(a);
	const auto crs_b = GetCRS(b);
	if (crs_a == crs_b) {
		return true;
	}
	if (crs_a.empty() || crs_b.empty()) {
		return false;
	}
	OGRSpatialReference srs_a;
	OGRSpatialReference srs_b;
	ParseSpatialRef(crs_a, srs_a);
	ParseSpatialRef(crs_b, srs_b);
	return srs_a.IsSame(&srs_b) != 0;
}

static bool NearlyEqual(double a, double b) {
	return std::fabs(a - b) <= 1e-9 * MaxValue(1.0, MaxValue(std::fabs(a), std::fabs(b)));
}

bool SameAlignment(GDALDataset &a, GDALDataset &b, string &reason) {
	if (!SameCRS(a, b)) {
		reason = "The rasters have different SRIDs";
		return false;
	}
	const GeoTransform ga(a);
	const GeoTransform gb(b);
	if (!NearlyEqual(ga.c[1], gb.c[1])) {
		reason = "The rasters have different scales on the X axis";
		return false;
	}
	if (!NearlyEqual(ga.c[5], gb.c[5])) {
		reason = "The rasters have different scales on the Y axis";
		return false;
	}
	if (!NearlyEqual(ga.c[2], gb.c[2])) {
		reason = "The rasters have different skews on the X axis";
		return false;
	}
	if (!NearlyEqual(ga.c[4], gb.c[4])) {
		reason = "The rasters have different skews on the Y axis";
		return false;
	}
	double col;
	double row;
	ga.ToPixel(gb.c[0], gb.c[3], col, row);
	if (std::fabs(col - std::round(col)) > 1e-6 || std::fabs(row - std::round(row)) > 1e-6) {
		reason = "The rasters (pixel corner coordinates) are not aligned";
		return false;
	}
	reason = "The rasters are aligned";
	return true;
}

//======================================================================================================================
// Geometry
//======================================================================================================================

OGRGeometryUniquePtr GeometryFromWKB(const string_t &wkb) {
	OGRGeometry *geom = nullptr;
	CPLErrorReset();
	const auto err = OGRGeometryFactory::createFromWkb(wkb.GetData(), nullptr, &geom, wkb.GetSize());
	OGRGeometryUniquePtr result(geom);
	if (err != OGRERR_NONE || !result) {
		ThrowGDALError("Could not convert geometry");
	}
	return result;
}

string GeometryToWKB(const OGRGeometry &geom) {
	string wkb(geom.WkbSize(), '\0');
	if (geom.exportToWkb(wkbNDR, reinterpret_cast<unsigned char *>(&wkb[0]), wkbVariantIso) != OGRERR_NONE) {
		ThrowGDALError("Could not convert geometry");
	}
	return wkb;
}

Value GeometryValue(const OGRGeometry &geom) {
	const auto wkb = GeometryToWKB(geom);
	return Value::GEOMETRY(const_data_ptr_cast(wkb.data()), wkb.size());
}

OGRGeometryUniquePtr MakePolygon(const double *xs, const double *ys, idx_t count) {
	std::unique_ptr<OGRLinearRing> ring(new OGRLinearRing());
	ring->setNumPoints(static_cast<int>(count + 1));
	for (idx_t i = 0; i < count; i++) {
		ring->setPoint(static_cast<int>(i), xs[i], ys[i]);
	}
	ring->setPoint(static_cast<int>(count), xs[0], ys[0]);
	std::unique_ptr<OGRPolygon> polygon(new OGRPolygon());
	polygon->addRingDirectly(ring.release());
	return OGRGeometryUniquePtr(polygon.release());
}

OGRGeometryUniquePtr PixelPolygon(const GeoTransform &gt, double x0, double y0, double x1, double y1) {
	double xs[4];
	double ys[4];
	gt.ToWorld(x0, y0, xs[0], ys[0]);
	gt.ToWorld(x1, y0, xs[1], ys[1]);
	gt.ToWorld(x1, y1, xs[2], ys[2]);
	gt.ToWorld(x0, y1, xs[3], ys[3]);
	return MakePolygon(xs, ys, 4);
}

void BurnGeometry(GDALDataset &ds, int32_t band, const OGRGeometry &geom, double value, bool all_touched) {
	GetBand(ds, band);
	int band_list[1] = {band};
	OGRGeometryH geoms[1] = {OGRGeometry::ToHandle(const_cast<OGRGeometry *>(&geom))};
	double values[1] = {value};
	CPLStringList options;
	if (all_touched) {
		options.SetNameValue("ALL_TOUCHED", "TRUE");
	}
	CPLErrorReset();
	CheckGDAL(GDALRasterizeGeometries(GDALDataset::ToHandle(&ds), 1, band_list, 1, geoms, nullptr, nullptr, values,
	                                  options.List(), nullptr, nullptr),
	          "Could not rasterize the geometry");
}

//======================================================================================================================
// Row-wise scalar function support
//======================================================================================================================

Call::Call(const vector<Param> &params_p, DataChunk &args_p, ExpressionState &state_p, Vector &result_p)
    : params(params_p), args(args_p), state(state_p), result(result_p), formats(args_p.ColumnCount()),
      slots(args_p.ColumnCount()), row(0), returned(false) {
	for (idx_t i = 0; i < args.ColumnCount(); i++) {
		args.data[i].ToUnifiedFormat(args.size(), formats[i]);
	}
}

idx_t Call::Find(const char *name) const {
	for (idx_t i = 0; i < params.size(); i++) {
		if (strcmp(params[i].name, name) == 0) {
			return i;
		}
	}
	return DConstants::INVALID_INDEX;
}

idx_t Call::Require(const char *name) const {
	const auto col = Find(name);
	if (col == DConstants::INVALID_INDEX) {
		throw InternalException("Raster function has no parameter named '%s'", name);
	}
	return col;
}

bool Call::IsNull(idx_t col) const {
	return !formats[col].validity.RowIsValid(formats[col].sel->get_index(row));
}

bool Call::Declares(const char *name) const {
	return Find(name) != DConstants::INVALID_INDEX;
}

bool Call::Has(const char *name) const {
	const auto col = Find(name);
	return col != DConstants::INVALID_INDEX && !IsNull(col);
}

int32_t Call::Int(const char *name) const {
	return Get<int32_t>(Require(name));
}

int32_t Call::Int(const char *name, int32_t fallback) const {
	return Has(name) ? Int(name) : fallback;
}

double Call::Double(const char *name) const {
	return Get<double>(Require(name));
}

double Call::Double(const char *name, double fallback) const {
	return Has(name) ? Double(name) : fallback;
}

bool Call::Bool(const char *name) const {
	return Get<bool>(Require(name));
}

bool Call::Bool(const char *name, bool fallback) const {
	return Has(name) ? Bool(name) : fallback;
}

string Call::String(const char *name) const {
	return Get<string_t>(Require(name)).GetString();
}

string Call::String(const char *name, const string &fallback) const {
	return Has(name) ? String(name) : fallback;
}

const string_t &Call::Blob(const char *name) const {
	return Get<string_t>(Require(name));
}

Value Call::GetValue(const char *name) const {
	return args.data[Require(name)].GetValue(row);
}

vector<double> Call::DoubleList(const char *name) const {
	vector<double> result;
	for (const auto &child : ListValue::GetChildren(GetValue(name))) {
		if (child.IsNull()) {
			throw InvalidInputException("NULL is not allowed in the '%s' list", name);
		}
		result.push_back(child.GetValue<double>());
	}
	return result;
}

vector<int32_t> Call::IntList(const char *name) const {
	vector<int32_t> result;
	for (const auto &child : ListValue::GetChildren(GetValue(name))) {
		if (child.IsNull()) {
			throw InvalidInputException("NULL is not allowed in the '%s' list", name);
		}
		result.push_back(child.GetValue<int32_t>());
	}
	return result;
}

vector<string> Call::StringList(const char *name) const {
	vector<string> result;
	for (const auto &child : ListValue::GetChildren(GetValue(name))) {
		if (child.IsNull()) {
			throw InvalidInputException("NULL is not allowed in the '%s' list", name);
		}
		result.push_back(StringValue::Get(child));
	}
	return result;
}

GDALDataset &Call::Raster(const char *name) {
	const auto col = Require(name);
	const auto &blob = Get<string_t>(col);
	auto &slot = slots[col];
	if (!slot.handle.dataset || slot.data != blob.GetData() || slot.size != blob.GetSize()) {
		slot.handle.Close();
		slot.handle = OpenRaster(blob);
		slot.data = blob.GetData();
		slot.size = blob.GetSize();
	}
	return *slot.handle.dataset;
}

GDALDatasetUniquePtr Call::RasterCopy(const char *name) {
	return CopyToMem(Raster(name));
}

GDALRasterBand &Call::Band(const char *raster_name, const char *band_name) {
	return GetBand(Raster(raster_name), Int(band_name, 1));
}

OGRGeometryUniquePtr Call::Geometry(const char *name) const {
	return GeometryFromWKB(Get<string_t>(Require(name)));
}

void Call::ReturnString(const string &value) {
	Return(StringVector::AddStringOrBlob(result, value));
}

void Call::ReturnRaster(GDALDataset &ds) {
	Return(SerializeRaster(ds, result));
}

void Call::ReturnGeometry(const OGRGeometry &geom) {
	const auto wkb = GeometryToWKB(geom);
	string_t blob;
	Geometry::FromBinary(string_t(wkb.data(), NumericCast<uint32_t>(wkb.size())), blob, result, true);
	Return(blob);
}

void Call::ReturnValue(const Value &value) {
	result.SetValue(row, value);
	returned = true;
}

ClientContext &Call::Context() const {
	return state.GetContext();
}

void ExecuteCall(const vector<Param> &params, raster_function_t function, DataChunk &args, ExpressionState &state,
                 Vector &result) {
	Call call(params, args, state, result);
	const auto all_constant = args.AllConstant();
	const auto count = all_constant ? MinValue<idx_t>(args.size(), 1) : args.size();

	result.SetVectorType(VectorType::FLAT_VECTOR);
	for (idx_t row = 0; row < count; row++) {
		call.row = row;
		call.returned = false;

		auto skip = false;
		for (idx_t col = 0; col < params.size(); col++) {
			if (!params[col].nullable && call.IsNull(col)) {
				skip = true;
				break;
			}
		}
		if (!skip) {
			CPLErrorReset();
			function(call);
		}
		if (!call.returned) {
			FlatVector::SetNull(result, row, true);
		}
	}
	if (all_constant) {
		result.SetVectorType(VectorType::CONSTANT_VECTOR);
	}
}

RasterFunction::RasterFunction(const char *name_p) : name(name_p), description(""), example("") {
}

RasterFunction &RasterFunction::Add(vector<Param> params, const LogicalType &return_type, raster_function_t function) {
	Variant variant;
	variant.params = make_shared_ptr<vector<Param>>(std::move(params));
	variant.return_type = return_type;
	variant.function = function;
	variants.push_back(std::move(variant));
	return *this;
}

RasterFunction &RasterFunction::AddOptional(const vector<Param> &required, const vector<Param> &optional,
                                            const LogicalType &return_type, raster_function_t function) {
	auto params = required;
	Add(params, return_type, function);
	for (const auto &param : optional) {
		params.push_back(param);
		Add(params, return_type, function);
	}
	return *this;
}

RasterFunction &RasterFunction::Describe(const char *description_p, const char *example_p) {
	description = description_p;
	example = example_p;
	return *this;
}

InsertionOrderPreservingMap<string> FunctionTags(ExtensionLoader &loader, CatalogType type, const char *name) {
	auto &db = loader.GetDatabaseInstance();
	auto &catalog = Catalog::GetSystemCatalog(db);
	const auto transaction = CatalogTransaction::GetSystemTransaction(db);
	auto &schema = catalog.GetSchema(transaction, DEFAULT_SCHEMA);
	const auto entry = schema.GetEntry(transaction, type, name);

	InsertionOrderPreservingMap<string> tags;
	if (entry) {
		return entry->Cast<FunctionEntry>().tags;
	}
	tags.insert("ext", "spatial");
	tags.insert("category", "raster");
	return tags;
}

void RasterFunction::Register(ExtensionLoader &loader) {
	const auto tags = FunctionTags(loader, CatalogType::SCALAR_FUNCTION_ENTRY, name);
	const auto clean_description = FunctionBuilder::RemoveIndentAndTrailingWhitespace(description);
	const auto clean_example = FunctionBuilder::RemoveIndentAndTrailingWhitespace(example);

	ScalarFunctionSet set(name);
	vector<FunctionDescription> descriptions;
	for (const auto &variant : variants) {
		const auto params = variant.params;
		const auto function = variant.function;

		vector<LogicalType> arguments;
		FunctionDescription documentation;
		auto has_nullable = false;
		for (const auto &param : *params) {
			arguments.push_back(param.type);
			documentation.parameter_names.emplace_back(param.name);
			documentation.parameter_types.push_back(param.type);
			has_nullable = has_nullable || param.nullable;
		}
		documentation.description = clean_description;
		documentation.examples.push_back(clean_example);
		descriptions.push_back(std::move(documentation));

		ScalarFunction overload(std::move(arguments), variant.return_type,
		                        [params, function](DataChunk &args, ExpressionState &state, Vector &result) {
			                        ExecuteCall(*params, function, args, state, result);
		                        });
		overload.SetFallible();
		if (has_nullable) {
			// A literal NULL in a nullable parameter must reach the function instead of folding the call to NULL
			overload.SetNullHandling(FunctionNullHandling::SPECIAL_HANDLING);
		}
		set.AddFunction(std::move(overload));
	}

	CreateScalarFunctionInfo info(std::move(set));
	info.on_conflict = OnCreateConflict::ALTER_ON_CONFLICT;
	info.descriptions = std::move(descriptions);
	loader.RegisterFunction(std::move(info));

	auto &db = loader.GetDatabaseInstance();
	auto &catalog = Catalog::GetSystemCatalog(db);
	const auto transaction = CatalogTransaction::GetSystemTransaction(db);
	auto &schema = catalog.GetSchema(transaction, DEFAULT_SCHEMA);
	auto entry = schema.GetEntry(transaction, CatalogType::SCALAR_FUNCTION_ENTRY, name);
	if (!entry) {
		throw InternalException("Function \"%s\" not found after registration", name);
	}
	entry->Cast<FunctionEntry>().tags = tags;
}

//======================================================================================================================
// Type registration and casts
//======================================================================================================================

static const char *const INVALID_RASTER_MESSAGE =
    "Could not cast BLOB to RASTER: not a GeoTIFF byte stream (use ST_FromGDALRaster to convert a raster file)";

static bool BlobToRasterCast(Vector &source, Vector &result, idx_t count, CastParameters &parameters) {
	UnifiedVectorFormat format;
	source.ToUnifiedFormat(count, format);
	const auto blobs = UnifiedVectorFormat::GetData<string_t>(format);

	vector<idx_t> invalid_rows;
	for (idx_t i = 0; i < count; i++) {
		const auto idx = format.sel->get_index(i);
		if (!format.validity.RowIsValid(idx)) {
			continue;
		}
		RasterHandle handle;
		if (!TryOpenRaster(blobs[idx].GetData(), blobs[idx].GetSize(), handle)) {
			invalid_rows.push_back(i);
		}
	}
	if (invalid_rows.empty()) {
		result.Reinterpret(source);
		return true;
	}

	HandleCastError::AssignError(INVALID_RASTER_MESSAGE, parameters);
	VectorOperations::Copy(source, result, count, 0, 0);
	result.Flatten(count);
	for (const auto row : invalid_rows) {
		FlatVector::SetNull(result, row, true);
	}
	return false;
}

static bool NullToRasterCast(Vector &source, Vector &result, idx_t count, CastParameters &parameters) {
	result.SetVectorType(VectorType::CONSTANT_VECTOR);
	ConstantVector::SetNull(result, true);
	return true;
}

static bool LiteralToRasterCast(Vector &source, Vector &result, idx_t count, CastParameters &parameters) {
	throw ConversionException("Cannot cast a string to RASTER, use ST_FromGDALRaster or ST_ReadRaster");
}

void RegisterRasterType(ExtensionLoader &loader) {
	const auto raster = RasterType();
	loader.RegisterType("RASTER", raster);

	loader.RegisterCastFunction(raster, LogicalType::BLOB, DefaultCasts::ReinterpretCast);
	loader.RegisterCastFunction(LogicalType::BLOB, raster, BoundCastInfo(BlobToRasterCast));

	// Untyped NULLs and string literals cast to every type at the same cost. Several raster functions share their name
	// with a geometry function, so make RASTER the more expensive target to keep those calls unambiguous
	loader.RegisterCastFunction(LogicalType::SQLNULL, raster, BoundCastInfo(NullToRasterCast), 111);
	const auto literal = LogicalType(LogicalTypeId::STRING_LITERAL);
	loader.RegisterCastFunction(literal, raster, BoundCastInfo(LiteralToRasterCast), 100);
	loader.RegisterCastFunction(literal, LogicalType::LIST(raster), BoundCastInfo(LiteralToRasterCast), 100);
}

} // namespace raster
} // namespace duckdb
