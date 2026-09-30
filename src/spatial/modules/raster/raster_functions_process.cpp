#include "spatial/modules/raster/raster_core.hpp"

#include "gdal_alg.h"
#include "gdal_utils.h"
#include "gdalwarper.h"

#include <algorithm>

namespace duckdb {
namespace raster {

namespace {

//======================================================================================================================
// Grids and warping
//======================================================================================================================

struct Grid {
	GeoTransform gt;
	int width;
	int height;
};

GDALResampleAlg ParseResampleAlgorithm(const string &name) {
	const auto lower = StringUtil::Lower(name);
	if (lower == "nearestneighbour" || lower == "nearestneighbor" || lower == "nearest" || lower == "near") {
		return GRA_NearestNeighbour;
	}
	if (lower == "bilinear") {
		return GRA_Bilinear;
	}
	if (lower == "cubic") {
		return GRA_Cubic;
	}
	if (lower == "cubicspline") {
		return GRA_CubicSpline;
	}
	if (lower == "lanczos") {
		return GRA_Lanczos;
	}
	if (lower == "average") {
		return GRA_Average;
	}
	if (lower == "mode") {
		return GRA_Mode;
	}
	if (lower == "max") {
		return GRA_Max;
	}
	if (lower == "min") {
		return GRA_Min;
	}
	throw InvalidInputException("Unknown resampling algorithm '%s', expected one of NearestNeighbor, Bilinear, Cubic, "
	                            "CubicSpline, Lanczos, Average, Mode, Max, Min",
	                            name);
}

OGREnvelope RasterEnvelope(GDALDataset &ds) {
	const GeoTransform gt(ds);
	OGREnvelope envelope;
	const double cols[2] = {0, static_cast<double>(ds.GetRasterXSize())};
	const double rows[2] = {0, static_cast<double>(ds.GetRasterYSize())};
	for (const auto col : cols) {
		for (const auto row : rows) {
			double x;
			double y;
			gt.ToWorld(col, row, x, y);
			envelope.Merge(x, y);
		}
	}
	return envelope;
}

void CheckGridSize(double width, double height) {
	if (!(width >= 1) || !(height >= 1) || width > 1e6 || height > 1e6 || width * height > 4e9) {
		throw InvalidInputException("The resulting raster would be %.0f x %.0f pixels, which is outside of the "
		                            "supported range",
		                            width, height);
	}
}

// The smallest grid with the given pixel vectors that covers the envelope and whose pixel corners lie on the lattice
// through the anchor point
Grid CoverEnvelope(const OGREnvelope &envelope, double scalex, double scaley, double skewx, double skewy,
                   double anchor_x, double anchor_y) {
	Grid grid;
	grid.gt.c[0] = anchor_x;
	grid.gt.c[1] = scalex;
	grid.gt.c[2] = skewx;
	grid.gt.c[3] = anchor_y;
	grid.gt.c[4] = skewy;
	grid.gt.c[5] = scaley;

	auto min_col = std::numeric_limits<double>::infinity();
	auto min_row = std::numeric_limits<double>::infinity();
	auto max_col = -std::numeric_limits<double>::infinity();
	auto max_row = -std::numeric_limits<double>::infinity();
	const double xs[2] = {envelope.MinX, envelope.MaxX};
	const double ys[2] = {envelope.MinY, envelope.MaxY};
	for (const auto x : xs) {
		for (const auto y : ys) {
			double col;
			double row;
			grid.gt.ToPixel(x, y, col, row);
			min_col = MinValue(min_col, col);
			max_col = MaxValue(max_col, col);
			min_row = MinValue(min_row, row);
			max_row = MaxValue(max_row, row);
		}
	}
	const auto first_col = static_cast<double>(PixelFloor(min_col));
	const auto first_row = static_cast<double>(PixelFloor(min_row));
	const auto width = MaxValue(1.0, std::ceil(max_col - 1e-7) - first_col);
	const auto height = MaxValue(1.0, std::ceil(max_row - 1e-7) - first_row);
	CheckGridSize(width, height);

	grid.gt.ToWorld(first_col, first_row, grid.gt.c[0], grid.gt.c[3]);
	grid.width = static_cast<int>(width);
	grid.height = static_cast<int>(height);
	return grid;
}

string ToWKT(const string &crs) {
	if (crs.empty()) {
		return string();
	}
	OGRSpatialReference srs;
	CPLErrorReset();
	if (srs.SetFromUserInput(crs.c_str()) != OGRERR_NONE) {
		ThrowGDALError(StringUtil::Format("Could not parse the coordinate system '%s'", crs));
	}
	char *wkt = nullptr;
	srs.exportToWkt(&wkt);
	const string result = wkt ? wkt : "";
	CPLFree(wkt);
	CPLErrorReset();
	return result;
}

// GDAL's warper needs a source that reports a georeference, which a raster with the identity georeference does not
struct WarpSource {
	GDALDatasetUniquePtr holder;
	GDALDataset *dataset;

	explicit WarpSource(GDALDataset &src) : dataset(&src) {
		double gt[6];
		if (src.GetGeoTransform(gt) != CE_None) {
			holder = CopyToMem(src);
			GeoTransform().Apply(*holder);
			dataset = holder.get();
		}
	}
};

// The grid GDAL suggests for the source reprojected to another coordinate system
Grid SuggestGrid(GDALDataset &src, const string &src_wkt, const string &dst_wkt) {
	WarpSource source(src);
	CPLErrorReset();
	const auto transformer = GDALCreateGenImgProjTransformer(GDALDataset::ToHandle(source.dataset), src_wkt.c_str(),
	                                                         nullptr, dst_wkt.c_str(), FALSE, 0, 0);
	if (!transformer) {
		ThrowGDALError("Could not set up the reprojection");
	}
	Grid grid;
	const auto err = GDALSuggestedWarpOutput(GDALDataset::ToHandle(source.dataset), GDALGenImgProjTransform,
	                                         transformer, grid.gt.c, &grid.width, &grid.height);
	GDALDestroyGenImgProjTransformer(transformer);
	CheckGDAL(err, "Could not compute the extent of the reprojected raster");
	CheckGridSize(grid.width, grid.height);
	return grid;
}

OGREnvelope GridEnvelope(const Grid &grid) {
	OGREnvelope envelope;
	const double cols[2] = {0, static_cast<double>(grid.width)};
	const double rows[2] = {0, static_cast<double>(grid.height)};
	for (const auto col : cols) {
		for (const auto row : rows) {
			double x;
			double y;
			grid.gt.ToWorld(col, row, x, y);
			envelope.Merge(x, y);
		}
	}
	return envelope;
}

GDALDatasetUniquePtr WarpToGrid(GDALDataset &src, const Grid &grid, const string &dst_crs, bool reproject,
                                GDALResampleAlg algorithm, double max_error) {
	const auto band_count = src.GetRasterCount();
	const auto type = band_count > 0 ? src.GetRasterBand(1)->GetRasterDataType() : GDT_Byte;
	auto result = CreateMemRaster(grid.width, grid.height, band_count, type);
	grid.gt.Apply(*result);
	SetCRSText(*result, dst_crs);
	if (band_count == 0) {
		return result;
	}
	for (int i = 1; i <= band_count; i++) {
		double nodata;
		if (GetNoData(*src.GetRasterBand(i), nodata)) {
			auto &band = *result->GetRasterBand(i);
			SetNoData(band, true, nodata);
			CheckGDAL(band.Fill(nodata), "Could not initialize the band");
		}
	}

	WarpSource source(src);
	string src_wkt;
	string dst_wkt;
	if (reproject) {
		src_wkt = ToWKT(GetCRS(src));
		dst_wkt = ToWKT(dst_crs);
	}
	CPLErrorReset();
	const auto err = GDALReprojectImage(GDALDataset::ToHandle(source.dataset), reproject ? src_wkt.c_str() : nullptr,
	                                    GDALDataset::ToHandle(result.get()), reproject ? dst_wkt.c_str() : nullptr,
	                                    algorithm, 0, MaxValue(0.0, max_error), nullptr, nullptr, nullptr);
	CheckGDAL(err, "Could not resample the raster");
	CPLErrorReset();
	return result;
}

struct WarpSettings {
	GDALResampleAlg algorithm;
	double max_error;

	explicit WarpSettings(Call &c)
	    : algorithm(ParseResampleAlgorithm(c.String("algorithm", "NearestNeighbour"))),
	      max_error(c.Double("maxerr", 0.125)) {
	}
};

void ResampleToReference(Call &c, GDALDataset &src, GDALDataset &ref, bool use_scale) {
	const WarpSettings settings(c);
	const auto src_crs = GetCRS(src);
	const auto ref_crs = GetCRS(ref);
	const auto reproject = !src_crs.empty() && !ref_crs.empty() && src_crs != ref_crs;

	auto envelope = RasterEnvelope(src);
	if (reproject) {
		envelope = GridEnvelope(SuggestGrid(src, ToWKT(src_crs), ToWKT(ref_crs)));
	}
	const GeoTransform src_gt(src);
	const GeoTransform ref_gt(ref);
	auto scalex = ref_gt.c[1];
	auto scaley = ref_gt.c[5];
	if (!use_scale) {
		scalex = std::copysign(std::fabs(src_gt.c[1]), ref_gt.c[1]);
		scaley = std::copysign(std::fabs(src_gt.c[5]), ref_gt.c[5]);
	}
	const auto grid = CoverEnvelope(envelope, scalex, scaley, ref_gt.c[2], ref_gt.c[4], ref_gt.c[0], ref_gt.c[3]);
	const auto result = WarpToGrid(src, grid, ref_crs.empty() ? src_crs : ref_crs, reproject, settings.algorithm,
	                               settings.max_error);
	c.ReturnRaster(*result);
}

double PositiveScale(Call &c, const char *name, double fallback) {
	const auto value = std::fabs(c.Double(name, 0));
	return value == 0 ? fallback : value;
}

// ST_Resample, ST_Rescale, ST_Reskew and ST_SnapToGrid differ only in which of these arguments they expose
void ResampleExecute(Call &c) {
	auto &src = c.Raster();
	if (c.Declares("ref")) {
		ResampleToReference(c, src, c.Raster("ref"), c.Bool("usescale", true));
		return;
	}
	const WarpSettings settings(c);
	const auto envelope = RasterEnvelope(src);
	const GeoTransform src_gt(src);

	auto scalex = std::fabs(src_gt.c[1]);
	auto scaley = std::fabs(src_gt.c[5]);
	if (c.Declares("width")) {
		CheckGridSize(c.Int("width"), c.Int("height"));
		scalex = (envelope.MaxX - envelope.MinX) / c.Int("width");
		scaley = (envelope.MaxY - envelope.MinY) / c.Int("height");
	} else if (c.Declares("scalexy")) {
		scalex = scaley = PositiveScale(c, "scalexy", scalex);
	} else {
		scalex = PositiveScale(c, "scalex", scalex);
		scaley = PositiveScale(c, "scaley", scaley);
	}
	const auto skewx = c.Declares("skewxy") ? c.Double("skewxy") : c.Double("skewx", 0);
	const auto skewy = c.Declares("skewxy") ? c.Double("skewxy") : c.Double("skewy", 0);

	auto grid = CoverEnvelope(envelope, scalex, -scaley, skewx, skewy, c.Double("gridx", envelope.MinX),
	                          c.Double("gridy", envelope.MaxY));
	if (c.Declares("width") && !c.Has("gridx") && !c.Has("gridy") && skewx == 0 && skewy == 0) {
		grid.width = c.Int("width");
		grid.height = c.Int("height");
	}
	const auto result = WarpToGrid(src, grid, GetCRS(src), false, settings.algorithm, settings.max_error);
	c.ReturnRaster(*result);
}

int ParseSize(const string &text, int current, const char *what) {
	auto trimmed = text;
	StringUtil::Trim(trimmed);
	char *end = nullptr;
	const auto number = strtod(trimmed.c_str(), &end);
	double size = number;
	if (end != trimmed.c_str() && *end == '%' && *(end + 1) == '\0') {
		size = std::round(current * number / 100.0);
	} else if (end == trimmed.c_str() || *end != '\0') {
		throw InvalidInputException("ST_Resize: invalid %s '%s', expected a number of pixels or a percentage such as "
		                            "'50%%'",
		                            what, text);
	}
	CheckGridSize(size, 1);
	return static_cast<int>(size);
}

void ResizeExecute(Call &c) {
	auto &src = c.Raster();
	const WarpSettings settings(c);
	int width;
	int height;
	if (c.Declares("percentwidth")) {
		const auto percent_width = c.Double("percentwidth");
		const auto percent_height = c.Double("percentheight");
		if (!(percent_width > 0) || !(percent_height > 0)) {
			throw InvalidInputException("ST_Resize: the fractions of the width and height must be greater than 0");
		}
		const auto new_width = std::round(src.GetRasterXSize() * percent_width);
		const auto new_height = std::round(src.GetRasterYSize() * percent_height);
		CheckGridSize(MaxValue(1.0, new_width), MaxValue(1.0, new_height));
		width = static_cast<int>(MaxValue(1.0, new_width));
		height = static_cast<int>(MaxValue(1.0, new_height));
	} else if (c.Declares("textwidth")) {
		width = ParseSize(c.String("textwidth"), src.GetRasterXSize(), "width");
		height = ParseSize(c.String("textheight"), src.GetRasterYSize(), "height");
	} else {
		width = c.Int("width");
		height = c.Int("height");
	}
	CheckGridSize(width, height);

	const auto envelope = RasterEnvelope(src);
	Grid grid;
	grid.width = width;
	grid.height = height;
	grid.gt.c[0] = envelope.MinX;
	grid.gt.c[3] = envelope.MaxY;
	grid.gt.c[1] = (envelope.MaxX - envelope.MinX) / width;
	grid.gt.c[5] = -(envelope.MaxY - envelope.MinY) / height;
	const auto result = WarpToGrid(src, grid, GetCRS(src), false, settings.algorithm, settings.max_error);
	c.ReturnRaster(*result);
}

void TransformExecute(Call &c) {
	auto &src = c.Raster();
	if (c.Declares("alignto")) {
		ResampleToReference(c, src, c.Raster("alignto"), true);
		return;
	}
	const WarpSettings settings(c);
	const auto src_crs = GetCRS(src);
	if (src_crs.empty()) {
		throw InvalidInputException("ST_Transform: the raster has no coordinate system, set one with ST_SetSRID or "
		                            "ST_SetCRS first");
	}

	// Normalise the target the same way ST_SetCRS does
	const auto scratch = CreateMemRaster(1, 1, 0, GDT_Byte);
	if (c.Declares("srid")) {
		if (c.Int("srid") <= 0) {
			throw InvalidInputException("ST_Transform: invalid SRID %d", c.Int("srid"));
		}
		SetSRID(*scratch, c.Int("srid"));
	} else {
		SetCRS(*scratch, c.String("crs"));
	}
	const auto dst_crs = GetCRS(*scratch);
	if (dst_crs.empty()) {
		throw InvalidInputException("ST_Transform: the target coordinate system is empty");
	}

	auto grid = SuggestGrid(src, ToWKT(src_crs), ToWKT(dst_crs));
	auto scalex = grid.gt.c[1];
	auto scaley = std::fabs(grid.gt.c[5]);
	auto rescale = false;
	if (c.Declares("scalexy")) {
		scalex = scaley = PositiveScale(c, "scalexy", scalex);
		rescale = true;
	} else if (c.Declares("scalex")) {
		scalex = PositiveScale(c, "scalex", scalex);
		scaley = PositiveScale(c, "scaley", scaley);
		rescale = true;
	}
	if (rescale) {
		const auto envelope = GridEnvelope(grid);
		grid = CoverEnvelope(envelope, scalex, -scaley, 0, 0, envelope.MinX, envelope.MaxY);
	}
	const auto result = WarpToGrid(src, grid, dst_crs, true, settings.algorithm, settings.max_error);
	c.ReturnRaster(*result);
}

//======================================================================================================================
// ST_Clip
//======================================================================================================================

void ClipExecute(Call &c) {
	auto &src = c.Raster();
	const auto geom = c.Geometry("geom");
	const auto crop = c.Bool("crop", true);
	const auto touched = c.Bool("touched", false);

	vector<int32_t> bands;
	if (c.Declares("band")) {
		bands.push_back(c.Int("band"));
	} else {
		for (int32_t i = 1; i <= src.GetRasterCount(); i++) {
			bands.push_back(i);
		}
	}
	for (const auto band : bands) {
		GetBand(src, band);
	}
	if (bands.empty()) {
		throw InvalidInputException("ST_Clip: the raster has no bands");
	}

	int x0 = 0;
	int y0 = 0;
	int x1 = src.GetRasterXSize();
	int y1 = src.GetRasterYSize();
	if (geom->IsEmpty()) {
		return;
	}
	if (crop) {
		OGREnvelope envelope;
		geom->getEnvelope(&envelope);
		const GeoTransform gt(src);
		auto min_col = std::numeric_limits<double>::infinity();
		auto min_row = std::numeric_limits<double>::infinity();
		auto max_col = -std::numeric_limits<double>::infinity();
		auto max_row = -std::numeric_limits<double>::infinity();
		const double xs[2] = {envelope.MinX, envelope.MaxX};
		const double ys[2] = {envelope.MinY, envelope.MaxY};
		for (const auto x : xs) {
			for (const auto y : ys) {
				double col;
				double row;
				gt.ToPixel(x, y, col, row);
				min_col = MinValue(min_col, col);
				max_col = MaxValue(max_col, col);
				min_row = MinValue(min_row, row);
				max_row = MaxValue(max_row, row);
			}
		}
		x0 = static_cast<int>(MaxValue<double>(x0, std::floor(min_col + 1e-7)));
		y0 = static_cast<int>(MaxValue<double>(y0, std::floor(min_row + 1e-7)));
		x1 = static_cast<int>(MinValue<double>(x1, std::ceil(max_col - 1e-7)));
		y1 = static_cast<int>(MinValue<double>(y1, std::ceil(max_row - 1e-7)));
		if (x1 <= x0 || y1 <= y0) {
			return;
		}
	}
	const auto width = x1 - x0;
	const auto height = y1 - y0;

	const auto selected = SelectBands(src, bands);
	const auto result = CopyWindow(*selected, x0, y0, width, height);

	const auto mask = CreateMemLike(*result, 1, GDT_Byte);
	BurnGeometry(*mask, 1, *geom, 1, touched);
	vector<GByte> inside(static_cast<idx_t>(width) * height);
	CPLErrorReset();
	CheckGDAL(mask->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, width, height, inside.data(), width, height, GDT_Byte, 0,
	                                           0, nullptr),
	          "Could not read the clip mask");

	for (int i = 1; i <= result->GetRasterCount(); i++) {
		auto &band = *result->GetRasterBand(i);
		double nodata;
		auto has_nodata = GetNoData(band, nodata);
		if (c.Has("nodataval")) {
			nodata = c.Double("nodataval");
			has_nodata = true;
		} else if (!has_nodata) {
			nodata = MinPossibleValue(band.GetRasterDataType());
			has_nodata = true;
		}
		SetNoData(band, true, nodata);

		auto values = ReadBand(band);
		for (idx_t p = 0; p < values.size(); p++) {
			if (!inside[p]) {
				values[p] = nodata;
			}
		}
		WriteBand(band, values);
	}
	c.ReturnRaster(*result);
}

//======================================================================================================================
// Terrain
//======================================================================================================================

constexpr double DEM_NODATA = -9999;

GDALDatasetUniquePtr RunDEMProcessing(GDALDataset &src, int32_t band, const char *mode, const vector<string> &extra,
                                      const char *color_file = nullptr) {
	GetBand(src, band);
	CPLStringList arguments;
	arguments.AddString("-of");
	arguments.AddString("MEM");
	arguments.AddString("-b");
	arguments.AddString(std::to_string(band).c_str());
	for (const auto &argument : extra) {
		arguments.AddString(argument.c_str());
	}
	CPLErrorReset();
	const auto options = GDALDEMProcessingOptionsNew(arguments.List(), nullptr);
	if (!options) {
		ThrowGDALError("Invalid terrain processing options");
	}
	int usage_error = 0;
	GDALDatasetUniquePtr result(GDALDataset::FromHandle(
	    GDALDEMProcessing("", GDALDataset::ToHandle(&src), mode, color_file, options, &usage_error)));
	GDALDEMProcessingOptionsFree(options);
	if (!result || usage_error) {
		ThrowGDALError(StringUtil::Format("Could not compute the %s of the raster", mode));
	}
	CPLErrorReset();
	return result;
}

double TerrainNoData(GDALDataType type) {
	switch (type) {
	case GDT_Byte:
		return 255;
	case GDT_Int8:
		return 127;
	case GDT_UInt16:
		return 65535;
	case GDT_UInt32:
		return 4294967295.0;
	default:
		return DEM_NODATA;
	}
}

// Single-band raster on the grid of `like`, from values where `source_nodata` marks the pixels without a result
GDALDatasetUniquePtr MakeTerrainRaster(GDALDataset &like, GDALDataType type, vector<double> &values,
                                       double source_nodata) {
	const auto nodata = TerrainNoData(type);
	for (auto &value : values) {
		if (value == source_nodata) {
			value = nodata;
		}
	}
	auto result = CreateMemLike(like, 1, type);
	auto &band = *result->GetRasterBand(1);
	SetNoData(band, true, nodata);
	WriteBand(band, values);
	return result;
}

void CheckInterpolateNoData(Call &c) {
	if (c.Bool("interpolate_nodata", false)) {
		throw NotImplementedException("interpolate_nodata is not supported: pixels next to NODATA pixels are NODATA");
	}
}

enum class TerrainUnits { DEGREES, RADIANS, PERCENT };

TerrainUnits ParseUnits(const string &units, bool allow_percent) {
	if (StringUtil::CIEquals(units, "DEGREES")) {
		return TerrainUnits::DEGREES;
	}
	if (StringUtil::CIEquals(units, "RADIANS")) {
		return TerrainUnits::RADIANS;
	}
	if (allow_percent && StringUtil::CIEquals(units, "PERCENT")) {
		return TerrainUnits::PERCENT;
	}
	throw InvalidInputException("Unknown units '%s', expected DEGREES%s or RADIANS", units,
	                            allow_percent ? ", PERCENT" : "");
}

void SlopeExecute(Call &c) {
	auto &src = c.Raster();
	CheckInterpolateNoData(c);
	const auto type = ParsePixelType(c.String("pixeltype", "32BF"));
	const auto units = ParseUnits(c.String("units", "DEGREES"), true);

	vector<string> arguments = {"-compute_edges", "-s", StringUtil::Format("%.17g", c.Double("scale", 1.0))};
	if (units == TerrainUnits::PERCENT) {
		arguments.push_back("-p");
	}
	const auto dem = RunDEMProcessing(src, c.Int("nband", 1), "slope", arguments);
	auto values = ReadBand(*dem->GetRasterBand(1));
	if (units == TerrainUnits::RADIANS) {
		for (auto &value : values) {
			if (value != DEM_NODATA) {
				value = value * M_PI / 180.0;
			}
		}
	}
	const auto result = MakeTerrainRaster(src, type, values, DEM_NODATA);
	c.ReturnRaster(*result);
}

void AspectExecute(Call &c) {
	auto &src = c.Raster();
	CheckInterpolateNoData(c);
	const auto type = ParsePixelType(c.String("pixeltype", "32BF"));
	const auto units = ParseUnits(c.String("units", "DEGREES"), false);
	const auto band_number = c.Int("nband", 1);

	const auto dem = RunDEMProcessing(src, band_number, "aspect", {"-compute_edges"});
	auto values = ReadBand(*dem->GetRasterBand(1));

	// GDAL reports both flat pixels and pixels next to NODATA as -9999. Tell them apart to return -1 for flat pixels
	const BandValues source(GetBand(src, band_number), true);
	const auto width = source.width;
	const auto height = source.height;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			auto &value = values[static_cast<idx_t>(y) * width + x];
			if (value != DEM_NODATA) {
				if (units == TerrainUnits::RADIANS) {
					value = value * M_PI / 180.0;
				}
				continue;
			}
			auto window_valid = true;
			for (int wy = MaxValue(0, y - 1); window_valid && wy <= MinValue(height - 1, y + 1); wy++) {
				for (int wx = MaxValue(0, x - 1); wx <= MinValue(width - 1, x + 1); wx++) {
					if (!source.IsValid(static_cast<idx_t>(wy) * width + wx)) {
						window_valid = false;
						break;
					}
				}
			}
			if (window_valid) {
				value = -1;
			}
		}
	}
	const auto result = MakeTerrainRaster(src, type, values, DEM_NODATA);
	c.ReturnRaster(*result);
}

void HillshadeExecute(Call &c) {
	auto &src = c.Raster();
	CheckInterpolateNoData(c);
	const auto type = ParsePixelType(c.String("pixeltype", "32BF"));
	const auto max_bright = c.Double("max_bright", 255.0);

	const vector<string> arguments = {"-compute_edges",
	                                  "-az",
	                                  StringUtil::Format("%.17g", c.Double("azimuth", 315.0)),
	                                  "-alt",
	                                  StringUtil::Format("%.17g", c.Double("altitude", 45.0)),
	                                  "-s",
	                                  StringUtil::Format("%.17g", c.Double("scale", 1.0))};
	const auto dem = RunDEMProcessing(src, c.Int("nband", 1), "hillshade", arguments);
	auto values = ReadBand(*dem->GetRasterBand(1));
	// GDAL returns 0 for NODATA and 1..255 for the illumination
	for (auto &value : values) {
		value = value == 0 ? DEM_NODATA : (value - 1.0) / 254.0 * max_bright;
	}
	const auto result = MakeTerrainRaster(src, type, values, DEM_NODATA);
	c.ReturnRaster(*result);
}

template <int MODE>
void RuggednessExecute(Call &c) {
	static const char *const MODES[] = {"TPI", "TRI", "roughness"};
	auto &src = c.Raster();
	CheckInterpolateNoData(c);
	const auto type = ParsePixelType(c.String("pixeltype", "32BF"));

	vector<string> arguments = {"-compute_edges"};
	if (MODE == 1) {
		// The mean absolute difference with the neighbours, as PostGIS computes it
		arguments.push_back("-alg");
		arguments.push_back("Wilson");
	}
	const auto dem = RunDEMProcessing(src, c.Int("nband", 1), MODES[MODE], arguments);
	auto values = ReadBand(*dem->GetRasterBand(1));
	const auto result = MakeTerrainRaster(src, type, values, DEM_NODATA);
	c.ReturnRaster(*result);
}

//======================================================================================================================
// ST_Reclass
//======================================================================================================================

struct ReclassRange {
	double src_min;
	double src_max;
	double dst_min;
	double dst_max;
	bool single;
	bool unbounded_min;
	bool unbounded_max;
	bool include_min;
	bool include_max;
};

[[noreturn]] void ThrowReclassError(const string &expression) {
	throw InvalidInputException("ST_Reclass: invalid reclass expression '%s', expected a comma-separated list of "
	                            "'range:map_range' such as '[0-100]:1-10, (100-200]:11-20, 255:0'",
	                            expression);
}

// Parses "a-b" or "a" where the numbers may be negative. Returns the number of values read
idx_t ParseNumberPair(const char *&cursor, double &first, double &second) {
	char *end = nullptr;
	first = strtod(cursor, &end);
	if (end == cursor) {
		return 0;
	}
	cursor = end;
	while (*cursor == ' ') {
		cursor++;
	}
	if (*cursor != '-') {
		second = first;
		return 1;
	}
	cursor++;
	second = strtod(cursor, &end);
	if (end == cursor) {
		return 0;
	}
	cursor = end;
	return 2;
}

vector<ReclassRange> ParseReclassExpression(const string &expression) {
	vector<ReclassRange> ranges;
	for (auto &item : StringUtil::Split(expression, ',')) {
		StringUtil::Trim(item);
		if (item.empty()) {
			continue;
		}
		ReclassRange range;
		const char *cursor = item.c_str();

		// PostGIS: a closing bracket in front of the minimum (or an opening one behind the maximum) leaves that side
		// of the range unbounded
		range.unbounded_min = *cursor == ')' || *cursor == ']';
		range.include_min = *cursor != '(';
		if (*cursor == '(' || *cursor == '[' || *cursor == ')' || *cursor == ']') {
			cursor++;
		}
		const auto source_count = ParseNumberPair(cursor, range.src_min, range.src_max);
		if (source_count == 0) {
			ThrowReclassError(expression);
		}
		range.single = source_count == 1;
		range.unbounded_max = *cursor == '(' || *cursor == '[';
		range.include_max = *cursor == ']';
		if (*cursor == '(' || *cursor == '[' || *cursor == ')' || *cursor == ']') {
			cursor++;
		}
		while (*cursor == ' ') {
			cursor++;
		}
		if (*cursor != ':') {
			ThrowReclassError(expression);
		}
		cursor++;
		while (*cursor == ' ') {
			cursor++;
		}
		if (ParseNumberPair(cursor, range.dst_min, range.dst_max) == 0 || *cursor != '\0') {
			ThrowReclassError(expression);
		}
		ranges.push_back(range);
	}
	if (ranges.empty()) {
		ThrowReclassError(expression);
	}
	return ranges;
}

bool ReclassMatches(const ReclassRange &range, double value) {
	if (range.single) {
		return value == range.src_min;
	}
	const auto above = range.unbounded_min || value > range.src_min || (range.include_min && value == range.src_min);
	const auto below = range.unbounded_max || value < range.src_max || (range.include_max && value == range.src_max);
	return above && below;
}

double ReclassMap(const ReclassRange &range, double value) {
	if (range.src_max == range.src_min) {
		return range.dst_min;
	}
	const auto mapped =
	    (value - range.src_min) * (range.dst_max - range.dst_min) / (range.src_max - range.src_min) + range.dst_min;
	const auto low = MinValue(range.dst_min, range.dst_max);
	const auto high = MaxValue(range.dst_min, range.dst_max);
	return MaxValue(low, MinValue(high, mapped));
}

void ReclassExecute(Call &c) {
	auto &src = c.Raster();
	const auto band_number = c.Int("nband", 1);
	const auto expression = c.String("reclassexpr");
	const auto ranges = ParseReclassExpression(expression);
	const auto type = ParsePixelType(c.String("pixeltype"));
	const auto has_nodata = c.Has("nodataval");
	const auto nodata = c.Double("nodataval", 0);
	const auto is_integer = type != GDT_Float32 && type != GDT_Float64;

	const BandValues source(GetBand(src, band_number), true);
	vector<double> values(source.values.size(), has_nodata ? nodata : 0);
	for (idx_t i = 0; i < values.size(); i++) {
		if (has_nodata && !source.IsValid(i)) {
			continue;
		}
		const auto value = source.values[i];
		for (const auto &range : ranges) {
			if (ReclassMatches(range, value)) {
				const auto mapped = ReclassMap(range, value);
				values[i] = is_integer ? std::round(mapped) : mapped;
				break;
			}
		}
	}

	const auto result = CreateMemLike(src, 0, GDT_Byte);
	for (int i = 1; i <= src.GetRasterCount(); i++) {
		if (i != band_number) {
			auto &other = *src.GetRasterBand(i);
			if (other.GetRasterDataType() != type) {
				throw InvalidInputException("ST_Reclass: all bands of a RASTER share one pixel type, but band %d is "
				                            "%s and the reclassified band is %s. Extract the band with ST_Band first",
				                            i, PixelTypeName(other.GetRasterDataType()), PixelTypeName(type));
			}
			AddBand(*result, type);
			CopyBand(other, *result->GetRasterBand(i));
			continue;
		}
		AddBand(*result, type);
		auto &band = *result->GetRasterBand(i);
		SetNoData(band, has_nodata, nodata);
		WriteBand(band, values);
	}
	c.ReturnRaster(*result);
}

//======================================================================================================================
// ST_ColorMap
//======================================================================================================================

struct ColorEntry {
	double value;
	bool is_percent;
	bool is_nodata;
	int components;
	double color[4];
};

const char *NamedColorMap(const string &name) {
	const auto lower = StringUtil::Lower(name);
	if (lower == "grayscale" || lower == "greyscale") {
		return "100% 0\n0% 254\nnv 255";
	}
	if (lower == "pseudocolor") {
		return "100% 255 0 0 255\n50% 0 255 0 255\n0% 0 0 255 255\nnv 0 0 0 0";
	}
	if (lower == "fire") {
		return "100% 243 255 221 255\n93.75% 242 255 178 255\n87.5% 255 255 135 255\n81.25% 255 228 96 255\n"
		       "75% 255 187 53 255\n68.75% 255 131 7 255\n62.5% 255 84 0 255\n56.25% 255 42 0 255\n"
		       "50% 255 0 0 255\n43.75% 255 42 0 255\n37.5% 224 74 0 255\n31.25% 183 91 0 255\n"
		       "25% 140 93 0 255\n18.75% 99 82 0 255\n12.5% 58 58 1 255\n6.25% 12 15 0 255\n"
		       "0% 0 0 0 255\nnv 0 0 0 0";
	}
	if (lower == "bluered") {
		return "100.00% 165 0 33 255\n94.12% 216 21 47 255\n88.24% 247 39 53 255\n82.35% 255 61 61 255\n"
		       "76.47% 255 120 86 255\n70.59% 255 172 117 255\n64.71% 255 214 153 255\n58.82% 255 241 188 255\n"
		       "52.94% 255 255 234 255\n47.06% 234 255 255 255\n41.18% 188 249 255 255\n35.29% 153 234 255 255\n"
		       "29.41% 117 211 255 255\n23.53% 86 176 255 255\n17.65% 61 135 255 255\n11.76% 40 87 255 255\n"
		       "5.88% 24 28 247 255\n0.00% 36 0 216 255\nnv 0 0 0 0";
	}
	return nullptr;
}

vector<string> ColorMapTokens(const string &line) {
	auto normalized = line;
	for (auto &ch : normalized) {
		if (ch == ':' || ch == ',' || ch == '\t' || ch == '\r') {
			ch = ' ';
		}
	}
	vector<string> tokens;
	for (auto &token : StringUtil::Split(normalized, ' ')) {
		if (!token.empty()) {
			tokens.push_back(token);
		}
	}
	return tokens;
}

vector<ColorEntry> ParseColorMap(const string &text) {
	vector<ColorEntry> entries;
	for (const auto &line : StringUtil::Split(text, '\n')) {
		const auto tokens = ColorMapTokens(line);
		if (tokens.empty()) {
			continue;
		}
		if (tokens.size() < 2 || tokens.size() > 5) {
			throw InvalidInputException("ST_ColorMap: invalid colormap entry '%s', expected a value followed by one to "
			                            "four colour components",
			                            line);
		}
		ColorEntry entry;
		entry.value = 0;
		entry.is_percent = false;
		const auto key = StringUtil::Lower(tokens[0]);
		entry.is_nodata = key == "nv" || key == "null" || key == "nodata";
		if (!entry.is_nodata) {
			char *end = nullptr;
			entry.value = strtod(key.c_str(), &end);
			if (end == key.c_str() || (*end != '\0' && strcmp(end, "%") != 0)) {
				throw InvalidInputException("ST_ColorMap: invalid colormap value '%s', expected a number, a "
				                            "percentage or 'nv'",
				                            tokens[0]);
			}
			entry.is_percent = *end == '%';
		}
		entry.components = static_cast<int>(tokens.size()) - 1;
		for (int i = 0; i < 4; i++) {
			entry.color[i] = 0;
		}
		for (int i = 0; i < entry.components; i++) {
			char *end = nullptr;
			const auto component = strtod(tokens[i + 1].c_str(), &end);
			if (end == tokens[i + 1].c_str() || *end != '\0' || component < 0 || component > 255) {
				throw InvalidInputException("ST_ColorMap: invalid colour component '%s', expected a number between 0 "
				                            "and 255",
				                            tokens[i + 1]);
			}
			entry.color[i] = component;
		}
		entries.push_back(entry);
	}
	return entries;
}

enum class ColorMethod { INTERPOLATE, EXACT, NEAREST };

void ColorMapExecute(Call &c) {
	auto &src = c.Raster();
	const BandValues source(GetBand(src, c.Int("nband", 1)), true);

	const auto colormap = c.String("colormap", "grayscale");
	const auto method_name = c.String("method", "INTERPOLATE");
	auto method = ColorMethod::INTERPOLATE;
	if (StringUtil::CIEquals(method_name, "EXACT")) {
		method = ColorMethod::EXACT;
	} else if (StringUtil::CIEquals(method_name, "NEAREST")) {
		method = ColorMethod::NEAREST;
	} else if (!StringUtil::CIEquals(method_name, "INTERPOLATE")) {
		throw InvalidInputException("ST_ColorMap: unknown method '%s', expected INTERPOLATE, EXACT or NEAREST",
		                            method_name);
	}

	// A single word is the name of a predefined colormap, which PostGIS always interpolates
	vector<ColorEntry> entries;
	const auto first_line = colormap.substr(0, colormap.find('\n'));
	if (ColorMapTokens(first_line).size() <= 1) {
		const auto named = NamedColorMap(first_line.empty() ? colormap : ColorMapTokens(colormap)[0]);
		if (!named) {
			throw InvalidInputException("ST_ColorMap: unknown colormap keyword '%s', expected grayscale, pseudocolor, "
			                            "fire, bluered or a custom colormap",
			                            colormap);
		}
		entries = ParseColorMap(named);
		method = ColorMethod::INTERPOLATE;
	} else {
		entries = ParseColorMap(colormap);
	}

	auto minimum = std::numeric_limits<double>::infinity();
	auto maximum = -std::numeric_limits<double>::infinity();
	for (idx_t i = 0; i < source.values.size(); i++) {
		if (source.IsValid(i)) {
			minimum = MinValue(minimum, source.values[i]);
			maximum = MaxValue(maximum, source.values[i]);
		}
	}

	int components = 0;
	ColorEntry nodata_entry;
	auto has_nodata_entry = false;
	vector<ColorEntry> ramp;
	for (auto &entry : entries) {
		components = MaxValue(components, entry.components);
		if (entry.is_nodata) {
			nodata_entry = entry;
			has_nodata_entry = true;
			continue;
		}
		if (entry.is_percent) {
			entry.value = minimum + (maximum - minimum) * entry.value / 100.0;
		}
		ramp.push_back(entry);
	}
	if (ramp.empty()) {
		throw InvalidInputException("ST_ColorMap: the colormap has no value entries");
	}
	std::stable_sort(ramp.begin(), ramp.end(),
	                 [](const ColorEntry &a, const ColorEntry &b) { return a.value < b.value; });

	const auto pixel_count = source.values.size();
	vector<vector<double>> output(components, vector<double>(pixel_count, 0));
	for (idx_t p = 0; p < pixel_count; p++) {
		const double *color = nullptr;
		double blended[4];
		if (!source.IsValid(p)) {
			if (has_nodata_entry) {
				color = nodata_entry.color;
			}
		} else {
			const auto value = source.values[p];
			// First entry whose value is not below the pixel value
			idx_t upper = 0;
			while (upper < ramp.size() && ramp[upper].value < value) {
				upper++;
			}
			if (method == ColorMethod::EXACT) {
				if (upper < ramp.size() && ramp[upper].value == value) {
					color = ramp[upper].color;
				}
			} else if (upper == 0) {
				color = ramp[0].color;
			} else if (upper == ramp.size()) {
				color = ramp.back().color;
			} else if (ramp[upper].value == value) {
				color = ramp[upper].color;
			} else {
				const auto &low = ramp[upper - 1];
				const auto &high = ramp[upper];
				const auto fraction = (value - low.value) / (high.value - low.value);
				if (method == ColorMethod::NEAREST) {
					color = fraction < 0.5 ? low.color : high.color;
				} else {
					for (int i = 0; i < 4; i++) {
						blended[i] = std::round(low.color[i] + fraction * (high.color[i] - low.color[i]));
					}
					color = blended;
				}
			}
		}
		if (color) {
			for (int i = 0; i < components; i++) {
				output[i][p] = color[i];
			}
		}
	}

	const auto result = CreateMemLike(src, components, GDT_Byte);
	for (int i = 0; i < components; i++) {
		WriteBand(*result->GetRasterBand(i + 1), output[i]);
	}
	c.ReturnRaster(*result);
}

//======================================================================================================================
// ST_Grayscale
//======================================================================================================================

void GrayscaleExecute(Call &c) {
	auto &src = c.Raster();
	const BandValues red(GetBand(src, c.Int("redband", 1)), true);
	const BandValues green(GetBand(src, c.Int("greenband", 2)), true);
	const BandValues blue(GetBand(src, c.Int("blueband", 3)), true);

	vector<double> values(red.values.size());
	for (idx_t i = 0; i < values.size(); i++) {
		const auto gray = 0.2989 * red.values[i] + 0.5870 * green.values[i] + 0.1140 * blue.values[i];
		values[i] = MaxValue(0.0, MinValue(255.0, std::round(gray)));
	}
	const auto result = CreateMemLike(src, 1, GDT_Byte);
	WriteBand(*result->GetRasterBand(1), values);
	c.ReturnRaster(*result);
}

//======================================================================================================================
// ST_Tile
//======================================================================================================================

void TileExecute(Call &c) {
	auto &src = c.Raster();
	const auto tile_width = c.Int("width");
	const auto tile_height = c.Int("height");
	if (tile_width <= 0 || tile_height <= 0) {
		throw InvalidInputException("ST_Tile: the tile width and height must be positive");
	}
	const auto pad = c.Bool("padwithnodata", false);

	GDALDatasetUniquePtr selected;
	GDALDataset *source = &src;
	if (c.Declares("nband") || c.Declares("nbands")) {
		vector<int32_t> bands;
		if (c.Declares("nbands")) {
			bands = c.IntList("nbands");
		} else {
			bands.push_back(c.Int("nband"));
		}
		selected = SelectBands(src, bands);
		source = selected.get();
	}

	const auto width = source->GetRasterXSize();
	const auto height = source->GetRasterYSize();
	vector<Value> tiles;
	for (int y = 0; y < height; y += tile_height) {
		for (int x = 0; x < width; x += tile_width) {
			const auto copy_width = MinValue(tile_width, width - x);
			const auto copy_height = MinValue(tile_height, height - y);
			auto tile = CopyWindow(*source, x, y, copy_width, copy_height);
			if (pad && (copy_width < tile_width || copy_height < tile_height)) {
				auto padded = CreateMemRaster(tile_width, tile_height, 0, GDT_Byte);
				CopyGeoreference(*tile, *padded);
				for (int i = 1; i <= tile->GetRasterCount(); i++) {
					auto &tile_band = *tile->GetRasterBand(i);
					AddBand(*padded, tile_band.GetRasterDataType());
					auto &band = *padded->GetRasterBand(i);
					double nodata;
					auto has_nodata = GetNoData(tile_band, nodata);
					if (c.Has("nodataval")) {
						nodata = c.Double("nodataval");
						has_nodata = true;
					} else if (!has_nodata) {
						nodata = MinPossibleValue(tile_band.GetRasterDataType());
						has_nodata = true;
					}
					SetNoData(band, true, nodata);
					CheckGDAL(band.Fill(nodata), "Could not initialize the band");
					const auto values = ReadBand(tile_band);
					WritePixels(band, 0, 0, copy_width, copy_height, values.data());
				}
				tile = std::move(padded);
			}
			tiles.push_back(RasterValue(*tile));
		}
	}
	c.ReturnValue(Value::LIST(RasterType(), std::move(tiles)));
}

} // namespace

//======================================================================================================================
// Registration
//======================================================================================================================

void RegisterRasterProcessingFunctions(ExtensionLoader &loader) {
	const auto RASTER = RasterType();
	const vector<Param> warp_options = {TextP("algorithm"), DblP("maxerr")};

	RasterFunction("ST_Resample")
	    .AddOptional({RastP(), DblP("scalex"), DblP("scaley")},
	                 {DblP("gridx", true), DblP("gridy", true), DblP("skewx"), DblP("skewy"), TextP("algorithm"),
	                  DblP("maxerr")},
	                 RASTER, ResampleExecute)
	    .AddOptional({RastP(), IntP("width"), IntP("height")},
	                 {DblP("gridx", true), DblP("gridy", true), DblP("skewx"), DblP("skewy"), TextP("algorithm"),
	                  DblP("maxerr")},
	                 RASTER, ResampleExecute)
	    .AddOptional({RastP(), RastP("ref")}, {TextP("algorithm"), DblP("maxerr"), BoolP("usescale")}, RASTER,
	                 ResampleExecute)
	    .AddOptional({RastP(), RastP("ref"), BoolP("usescale")}, warp_options, RASTER, ResampleExecute)
	    .Describe(R"(
		Resamples a raster onto another pixel grid that covers the same area, and returns the new raster.

		The target grid is given by a pixel size (`scalex`, `scaley`, in world units; 0 keeps the current size and the sign is ignored), by a size in pixels (`width`, `height`), or by a reference raster `ref` whose alignment, skew, coordinate system and (unless `usescale` is false) pixel size are used. `gridx`/`gridy` are the world coordinates of any pixel corner of the target grid (default: the upper-left corner of the raster's extent) and `skewx`/`skewy` its skew (default 0). The result is north-up (negative `scaley`) unless a reference raster or a skew says otherwise.

		`algorithm` is one of `NearestNeighbor` (the default), `Bilinear`, `Cubic`, `CubicSpline`, `Lanczos`, `Average`, `Mode`, `Max`, `Min`. `maxerr` is the error, in pixels, tolerated by the approximation of the coordinate transformation (default 0.125). Pixels outside of the source, and NODATA pixels, are NODATA in the result when the band has a NODATA value, and 0 otherwise.
	)",
	              R"(
		SELECT ST_Width(r), ST_ScaleX(r) FROM (SELECT ST_Resample(ST_AddBand(ST_MakeEmptyRaster(10, 10, 0, 0, 1), '8BUI', 1), 2.0, 2.0) AS r);
		----
		5	2.0
	)")
	    .Register(loader);

	RasterFunction("ST_Rescale")
	    .AddOptional({RastP(), DblP("scalexy")}, warp_options, RASTER, ResampleExecute)
	    .AddOptional({RastP(), DblP("scalex"), DblP("scaley")}, warp_options, RASTER, ResampleExecute)
	    .Describe(R"(
		Resamples a raster to a new pixel size, in world units, keeping its extent and upper-left corner. The sign of the scale is ignored: the result is north-up.

		See ST_Resample for `algorithm` and `maxerr`. Use ST_SetScale to change the georeference without resampling.
	)",
	              R"(
		SELECT ST_Width(ST_Rescale(ST_AddBand(ST_MakeEmptyRaster(10, 10, 0, 0, 1), '8BUI', 1), 0.5, 'Bilinear'));
		----
		20
	)")
	    .Register(loader);

	RasterFunction("ST_Reskew")
	    .AddOptional({RastP(), DblP("skewxy")}, warp_options, RASTER, ResampleExecute)
	    .AddOptional({RastP(), DblP("skewx"), DblP("skewy")}, warp_options, RASTER, ResampleExecute)
	    .Describe(R"(
		Resamples a raster onto a grid with the given skew (rotation terms), keeping the pixel size and covering the same extent.

		See ST_Resample for `algorithm` and `maxerr`. Use ST_SetSkew to change the georeference without resampling.
	)",
	              R"(
		SELECT ST_SkewX(ST_Reskew(ST_AddBand(ST_MakeEmptyRaster(10, 10, 0, 0, 1), '8BUI', 1), 0.1, 0.1));
		----
		0.1
	)")
	    .Register(loader);

	RasterFunction("ST_SnapToGrid")
	    .AddOptional({RastP(), DblP("gridx"), DblP("gridy")},
	                 {TextP("algorithm"), DblP("maxerr"), DblP("scalex"), DblP("scaley")}, RASTER, ResampleExecute)
	    .AddOptional({RastP(), DblP("gridx"), DblP("gridy"), DblP("scalex"), DblP("scaley")}, warp_options, RASTER,
	                 ResampleExecute)
	    .AddOptional({RastP(), DblP("gridx"), DblP("gridy"), DblP("scalexy")}, warp_options, RASTER, ResampleExecute)
	    .Describe(R"(
		Resamples a raster so that its pixel corners fall on the grid that passes through the world coordinate (`gridx`, `gridy`), optionally with a new pixel size. The result covers the extent of the input.

		See ST_Resample for `algorithm` and `maxerr`.
	)",
	              R"(
		SELECT ST_UpperLeftX(ST_SnapToGrid(ST_AddBand(ST_MakeEmptyRaster(10, 10, 0.5, 0, 1), '8BUI', 1), 0, 0));
		----
		0.0
	)")
	    .Register(loader);

	RasterFunction("ST_Resize")
	    .AddOptional({RastP(), IntP("width"), IntP("height")}, warp_options, RASTER, ResizeExecute)
	    .AddOptional({RastP(), DblP("percentwidth"), DblP("percentheight")}, warp_options, RASTER, ResizeExecute)
	    .AddOptional({RastP(), TextP("textwidth"), TextP("textheight")}, warp_options, RASTER, ResizeExecute)
	    .Describe(R"(
		Resamples a raster to a new width and height, keeping its extent.

		The size is given in pixels (integers), as fractions of the current size (doubles: 0.5 halves the size), or as text holding either a number of pixels or a percentage (`'50%'`). The result is north-up. See ST_Resample for `algorithm` and `maxerr`.
	)",
	              R"(
		SELECT ST_Width(r), ST_Height(r) FROM (SELECT ST_Resize(ST_AddBand(ST_MakeEmptyRaster(10, 10, 0, 0, 1), '8BUI', 1), '50%', '20') AS r);
		----
		5	20
	)")
	    .Register(loader);

	RasterFunction("ST_Transform")
	    .AddOptional({RastP(), TextP("crs")}, {TextP("algorithm"), DblP("maxerr"), DblP("scalex"), DblP("scaley")},
	                 RASTER, TransformExecute)
	    .AddOptional({RastP(), IntP("srid")}, {TextP("algorithm"), DblP("maxerr"), DblP("scalex"), DblP("scaley")},
	                 RASTER, TransformExecute)
	    .AddOptional({RastP(), IntP("srid"), DblP("scalex"), DblP("scaley")}, warp_options, RASTER, TransformExecute)
	    .AddOptional({RastP(), IntP("srid"), DblP("scalexy")}, warp_options, RASTER, TransformExecute)
	    .AddOptional({RastP(), RastP("alignto")}, warp_options, RASTER, TransformExecute)
	    .Describe(R"(
		Reprojects a raster to another coordinate system and returns the new raster.

		The target is an EPSG code (`srid`), a coordinate system string (`crs`: `AUTHORITY:CODE`, WKT or PROJ), or a raster `alignto` whose coordinate system, pixel size and grid alignment are used. Unless `scalex`/`scaley` are given, the pixel size and extent of the result are chosen by GDAL so that the resolution of the input is preserved; the result is north-up. Coordinates are always in X/longitude, Y/latitude order.

		See ST_Resample for `algorithm` and `maxerr`. The raster must have a coordinate system (see ST_SetSRID).
	)",
	              R"(
		SELECT ST_SRID(ST_Transform(ST_AddBand(ST_MakeEmptyRaster(10, 10, 4.3, 50.9, 0.01, -0.01, 0, 0, 4326), '8BUI', 1), 'EPSG:3857', 'Bilinear'));
		----
		3857
	)")
	    .Register(loader);

	RasterFunction("ST_Clip")
	    .AddOptional({RastP(), GeomP("geom")}, {DblP("nodataval", true), BoolP("crop"), BoolP("touched")}, RASTER,
	                 ClipExecute)
	    .AddOptional({RastP(), GeomP("geom"), BoolP("crop")}, {BoolP("touched")}, RASTER, ClipExecute)
	    .AddOptional({RastP(), IntP("band"), GeomP("geom")}, {DblP("nodataval", true), BoolP("crop"), BoolP("touched")},
	                 RASTER, ClipExecute)
	    .AddOptional({RastP(), IntP("band"), GeomP("geom"), BoolP("crop")}, {BoolP("touched")}, RASTER, ClipExecute)
	    .Describe(R"(
		Returns the raster clipped by a geometry: pixels outside of the geometry become NODATA.

		With `band` (1-based) the result has only that band; otherwise all bands are clipped. `nodataval` is the NODATA value of the result; by default the band's own NODATA value is used, or the smallest value of the pixel type if it has none. With `crop` (the default) the result is cut to the pixels that intersect the bounding box of the geometry; otherwise it keeps the size of the input. A pixel is inside when its centre is inside the geometry, or, if `touched` is true, when the geometry touches it.

		The geometry must be in the coordinate system of the raster. Returns NULL if the geometry is empty or does not overlap the raster (PostGIS returns an empty raster). The variants taking lists of bands and NODATA values are not available.
	)",
	              R"(
		SELECT ST_Width(r), ST_Height(r) FROM (SELECT ST_Clip(ST_AddBand(ST_MakeEmptyRaster(10, 10, 0, 10, 1), '8BUI', 1, 0), ST_MakeEnvelope(2, 2, 5, 6)) AS r);
		----
		3	4
	)")
	    .Register(loader);

	const char *terrain_notes =
	    "Computed by GDAL's DEM processing on the 3x3 neighbourhood of each pixel; pixels on the border are computed "
	    "from an extrapolated neighbourhood. `nband` is 1-based (default 1) and `pixeltype` the pixel type of the "
	    "result (default `32BF`). Pixels whose neighbourhood contains a NODATA pixel are NODATA in the result (-9999, "
	    "or the largest value of the pixel type if it cannot hold -9999); PostGIS instead substitutes the centre "
	    "value. `interpolate_nodata` must be false and the `customextent` variants are not available.";

	const auto slope_description = StringUtil::Format(R"(
		Returns the slope of an elevation band, using Horn's formula.

		`units` is `DEGREES` (the default), `RADIANS` or `PERCENT`. `scale` is the ratio of vertical units to horizontal units (default 1; use 111120 for elevations in metres on a longitude/latitude grid).

		%s
	)",
	                                                  terrain_notes);
	RasterFunction("ST_Slope")
	    .AddOptional({RastP()},
	                 {IntP("nband"), TextP("pixeltype"), TextP("units"), DblP("scale"), BoolP("interpolate_nodata")},
	                 RASTER, SlopeExecute)
	    .Describe(slope_description.c_str(),
	              R"(
		SELECT round(ST_Value(ST_Slope(ST_SetValue(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(3, 3, 0, 3, 1), '32BF'), ST_MakeEnvelope(1, 0, 2, 3), 1), ST_MakeEnvelope(2, 0, 3, 3), 2), 1, '32BF', 'DEGREES'), 2, 2), 3);
		----
		45.0
	)")
	    .Register(loader);

	const auto aspect_description = StringUtil::Format(R"(
		Returns the aspect of an elevation band: the compass direction the slope faces, measured clockwise from north (0 = north, 90 = east, 180 = south, 270 = west). Flat pixels are -1.

		`units` is `DEGREES` (the default) or `RADIANS`.

		%s
	)",
	                                                   terrain_notes);
	RasterFunction("ST_Aspect")
	    .AddOptional({RastP()}, {IntP("nband"), TextP("pixeltype"), TextP("units"), BoolP("interpolate_nodata")},
	                 RASTER, AspectExecute)
	    .Describe(aspect_description.c_str(),
	              R"(
		SELECT ST_Value(ST_Aspect(ST_SetValue(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(3, 3, 0, 3, 1), '32BF'), ST_MakeEnvelope(1, 0, 2, 3), 1), ST_MakeEnvelope(2, 0, 3, 3), 2)), 2, 2);
		----
		270.0
	)")
	    .Register(loader);

	const auto hillshade_description = StringUtil::Format(R"(
		Returns the hypothetical illumination of an elevation band.

		`azimuth` is the direction of the light source in degrees clockwise from north (default 315) and `altitude` its angle above the horizon in degrees (default 45). `max_bright` is the value of a fully lit pixel (default 255) and `scale` the ratio of vertical units to horizontal units (default 1). GDAL computes the illumination as an 8-bit value, so the result has 255 distinct levels between 0 and `max_bright`.

		%s
	)",
	                                                      terrain_notes);
	RasterFunction("ST_Hillshade")
	    .AddOptional({RastP()},
	                 {IntP("nband"), TextP("pixeltype"), DblP("azimuth"), DblP("altitude"), DblP("max_bright"),
	                  DblP("scale"), BoolP("interpolate_nodata")},
	                 RASTER, HillshadeExecute)
	    .Describe(hillshade_description.c_str(),
	              R"(
		SELECT round(ST_Value(ST_Hillshade(ST_AddBand(ST_MakeEmptyRaster(5, 5, 0, 5, 1), '32BF', 10)), 3, 3));
		----
		180.0
	)")
	    .Register(loader);

	const auto tpi_description = StringUtil::Format(R"(
		Returns the Topographic Position Index of an elevation band: the value of each pixel minus the mean of its eight neighbours.

		%s
	)",
	                                                terrain_notes);
	RasterFunction("ST_TPI")
	    .AddOptional({RastP()}, {IntP("nband"), TextP("pixeltype"), BoolP("interpolate_nodata")}, RASTER,
	                 RuggednessExecute<0>)
	    .Describe(tpi_description.c_str(),
	              R"(
		SELECT ST_Value(ST_TPI(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(3, 3, 0, 3, 1), '32BF', 1), 2, 2, 9)), 2, 2);
		----
		8.0
	)")
	    .Register(loader);

	const auto tri_description = StringUtil::Format(R"(
		Returns the Terrain Ruggedness Index of an elevation band: the mean absolute difference between each pixel and its eight neighbours.

		%s
	)",
	                                                terrain_notes);
	RasterFunction("ST_TRI")
	    .AddOptional({RastP()}, {IntP("nband"), TextP("pixeltype"), BoolP("interpolate_nodata")}, RASTER,
	                 RuggednessExecute<1>)
	    .Describe(tri_description.c_str(),
	              R"(
		SELECT ST_Value(ST_TRI(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(3, 3, 0, 3, 1), '32BF', 1), 2, 2, 9)), 2, 2);
		----
		8.0
	)")
	    .Register(loader);

	const auto roughness_description = StringUtil::Format(R"(
		Returns the roughness of an elevation band: the difference between the largest and the smallest value in the 3x3 neighbourhood of each pixel.

		%s
	)",
	                                                      terrain_notes);
	RasterFunction("ST_Roughness")
	    .AddOptional({RastP()}, {IntP("nband"), TextP("pixeltype"), BoolP("interpolate_nodata")}, RASTER,
	                 RuggednessExecute<2>)
	    .Describe(roughness_description.c_str(),
	              R"(
		SELECT ST_Value(ST_Roughness(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(3, 3, 0, 3, 1), '32BF', 1), 2, 2, 9)), 2, 2);
		----
		8.0
	)")
	    .Register(loader);

	RasterFunction("ST_Reclass")
	    .AddOptional({RastP(), IntP("nband"), TextP("reclassexpr"), TextP("pixeltype")}, {DblP("nodataval", true)},
	                 RASTER, ReclassExecute)
	    .Add({RastP(), TextP("reclassexpr"), TextP("pixeltype")}, RASTER, ReclassExecute)
	    .Describe(R"(
		Returns the raster with the values of band `nband` (1-based, default 1) mapped to new values, stored with a new pixel type.

		`reclassexpr` is a comma-separated list of `range:map_range` entries. A range is a single value or `min-max`; its values are mapped linearly onto `map_range` and the first matching entry wins. The minimum is included unless the range starts with `(`, the maximum is excluded unless the range ends with `]`: `[0-100]` is 0 <= x <= 100, `(0-100]` is 0 < x <= 100, `0-100` and `[0-100)` are 0 <= x < 100. Negative numbers are written as is: `-10--5:1-2`. For integer pixel types the result is rounded.

		Pixels that match no entry, and NODATA pixels, are set to `nodataval`, which becomes the NODATA value of the band. Without `nodataval` the band has no NODATA value, unmatched pixels are 0 and NODATA pixels are reclassified like any other value. The other bands are unchanged and must have the same pixel type as the result. The `reclassarg[]` variant of PostGIS is not available: nest calls instead.
	)",
	              R"(
		SELECT ST_DumpValues(ST_Reclass(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(2, 1, 0, 0, 1), '32BF', 50), 2, 1, 150), 1, '[0-100]:1-11, (100-200]:20', '8BUI', 0));
		----
		[[6.0, 20.0]]
	)")
	    .Register(loader);

	RasterFunction("ST_ColorMap")
	    .AddOptional({RastP()}, {IntP("nband"), TextP("colormap"), TextP("method")}, RASTER, ColorMapExecute)
	    .AddOptional({RastP(), TextP("colormap")}, {TextP("method")}, RASTER, ColorMapExecute)
	    .Describe(R"(
		Returns a raster of up to four `8BUI` bands (grey, RGB or RGBA) that renders band `nband` (1-based, default 1) with a colormap.

		`colormap` is the name of a predefined colormap (`grayscale`, `pseudocolor`, `fire`, `bluered`) or a custom colormap: one entry per line, each a value followed by one to four colour components between 0 and 255 (separated by spaces, commas or colons). The value is a pixel value, a percentage of the range between the smallest and largest pixel value (`50%`), or `nv` for NODATA pixels. The result has as many bands as the longest entry.

		`method` is `INTERPOLATE` (the default: colours are blended linearly between entries, and values beyond the first or last entry take its colour), `EXACT` (only pixels equal to an entry are coloured) or `NEAREST` (the colour of the closest entry). Pixels without a colour are 0 in all bands. Predefined colormaps are always interpolated.
	)",
	              R"(
		SELECT ST_NumBands(ST_ColorMap(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '8BUI', 5), 1, 'fire'));
		----
		4
	)")
	    .Register(loader);

	RasterFunction("ST_Grayscale")
	    .AddOptional({RastP()}, {IntP("redband"), IntP("greenband"), IntP("blueband")}, RASTER, GrayscaleExecute)
	    .Describe(R"(
		Returns a single-band `8BUI` raster with the luminance of three bands holding red, green and blue values between 0 and 255: `0.2989 * R + 0.5870 * G + 0.1140 * B`.

		The band numbers are 1-based and default to 1, 2 and 3. The `rastbandarg[]` variant of PostGIS and the `extenttype` argument are not available: the three bands come from one raster.
	)",
	              R"(
		SELECT ST_Value(ST_Grayscale(ST_AddBand(ST_AddBand(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '8BUI', 255), '8BUI', 0), '8BUI', 0)), 1, 1);
		----
		76.0
	)")
	    .Register(loader);

	const auto raster_list = LogicalType::LIST(RASTER);
	const vector<Param> tile_options = {BoolP("padwithnodata"), DblP("nodataval", true)};
	RasterFunction("ST_Tile")
	    .AddOptional({RastP(), IntP("width"), IntP("height")}, tile_options, raster_list, TileExecute)
	    .AddOptional({RastP(), IntP("nband"), IntP("width"), IntP("height")}, tile_options, raster_list, TileExecute)
	    .AddOptional({RastP(), Param("nbands", LogicalType::LIST(LogicalType::INTEGER)), IntP("width"), IntP("height")},
	                 tile_options, raster_list, TileExecute)
	    .Describe(R"(
		Splits a raster into tiles of `width` x `height` pixels and returns them as a list, row by row from the upper-left corner. PostGIS returns a set of rows: use `UNNEST` to get the same shape.

		With `nband` or `nbands` (1-based) the tiles only have those bands. Tiles on the right and bottom edges are smaller unless `padwithnodata` is true, in which case they are padded with `nodataval` (default: the band's NODATA value, or the smallest value of the pixel type).
	)",
	              R"(
		SELECT len(ST_Tile(ST_AddBand(ST_MakeEmptyRaster(10, 10, 0, 0, 1), '8BUI'), 4, 4));
		----
		9
	)")
	    .Register(loader);
}

} // namespace raster
} // namespace duckdb
