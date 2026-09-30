#include "spatial/modules/raster/raster_core.hpp"

#include "gdal_alg.h"
#include "gdalgrid.h"
#include "ogrsf_frmts.h"

namespace duckdb {
namespace raster {

namespace {

//======================================================================================================================
// Vector layers in memory
//======================================================================================================================

struct Feature {
	OGRGeometryUniquePtr geom;
	double value;
	int64_t id;
};

// An OGR layer in memory that GDAL's algorithms write features with an id and a value to
struct FeatureSink {
	GDALDatasetUniquePtr dataset;
	OGRLayer *layer;

	FeatureSink() : layer(nullptr) {
		const auto driver = GetGDALDriverManager()->GetDriverByName("Memory");
		if (!driver) {
			throw InvalidInputException("The GDAL 'Memory' driver is not available");
		}
		CPLErrorReset();
		dataset.reset(driver->Create("", 0, 0, 0, GDT_Unknown, nullptr));
		if (dataset) {
			layer = dataset->CreateLayer("features", nullptr, wkbUnknown, nullptr);
		}
		if (!layer) {
			ThrowGDALError("Could not create an in-memory vector layer");
		}
		OGRFieldDefn id_field("id", OFTInteger64);
		OGRFieldDefn value_field("val", OFTReal);
		if (layer->CreateField(&id_field) != OGRERR_NONE || layer->CreateField(&value_field) != OGRERR_NONE) {
			ThrowGDALError("Could not create an in-memory vector layer");
		}
	}

	static constexpr int ID_FIELD = 0;
	static constexpr int VALUE_FIELD = 1;

	vector<Feature> Take() {
		vector<Feature> features;
		layer->ResetReading();
		while (true) {
			const std::unique_ptr<OGRFeature> feature(layer->GetNextFeature());
			if (!feature) {
				break;
			}
			Feature entry;
			entry.geom.reset(feature->StealGeometry());
			entry.value = feature->GetFieldAsDouble(VALUE_FIELD);
			entry.id = feature->GetFieldAsInteger64(ID_FIELD);
			if (entry.geom) {
				features.push_back(std::move(entry));
			}
		}
		return features;
	}
};

bool HoldsIntegers(GDALDataType type) {
	return type == GDT_Byte || type == GDT_Int8 || type == GDT_UInt16 || type == GDT_Int16 || type == GDT_Int32;
}

// One polygon per connected area of equal pixel values
vector<Feature> PolygonizeBand(GDALRasterBand &band, bool exclude_nodata) {
	FeatureSink sink;
	const auto mask = exclude_nodata ? band.GetMaskBand() : nullptr;
	CPLErrorReset();
	// The integer variant is exact; the other one reads the pixels as 32-bit floats
	const auto err = HoldsIntegers(band.GetRasterDataType())
	                     ? GDALPolygonize(GDALRasterBand::ToHandle(&band), GDALRasterBand::ToHandle(mask), sink.layer,
	                                      FeatureSink::VALUE_FIELD, nullptr, nullptr, nullptr)
	                     : GDALFPolygonize(GDALRasterBand::ToHandle(&band), GDALRasterBand::ToHandle(mask), sink.layer,
	                                       FeatureSink::VALUE_FIELD, nullptr, nullptr, nullptr);
	CheckGDAL(err, "Could not polygonize the raster");
	return sink.Take();
}

// The area of a raster as a geometry: its outline (band 0), or the pixels of a band that are not NODATA. Returns
// nullptr if there is no such pixel
OGRGeometryUniquePtr Footprint(GDALDataset &ds, int32_t band_number) {
	if (band_number == 0) {
		return PixelPolygon(GeoTransform(ds), 0, 0, ds.GetRasterXSize(), ds.GetRasterYSize());
	}
	const BandValues band(GetBand(ds, band_number), true);
	if (!band.has_nodata) {
		return PixelPolygon(GeoTransform(ds), 0, 0, ds.GetRasterXSize(), ds.GetRasterYSize());
	}
	vector<double> valid(band.values.size());
	auto any_valid = false;
	for (idx_t i = 0; i < valid.size(); i++) {
		valid[i] = band.IsValid(i) ? 1 : 0;
		any_valid = any_valid || band.IsValid(i);
	}
	if (!any_valid) {
		return nullptr;
	}
	const auto mask = CreateMemLike(ds, 1, GDT_Byte);
	// The polygonizer needs a georeference, which a raster with the identity georeference does not report
	GeoTransform(ds).Apply(*mask);
	auto &mask_band = *mask->GetRasterBand(1);
	WriteBand(mask_band, valid);

	FeatureSink sink;
	CPLErrorReset();
	CheckGDAL(GDALPolygonize(GDALRasterBand::ToHandle(&mask_band), GDALRasterBand::ToHandle(&mask_band), sink.layer,
	                         FeatureSink::VALUE_FIELD, nullptr, nullptr, nullptr),
	          "Could not polygonize the raster");
	auto features = sink.Take();
	if (features.empty()) {
		return nullptr;
	}
	if (features.size() == 1) {
		return std::move(features[0].geom);
	}
	std::unique_ptr<OGRMultiPolygon> result(new OGRMultiPolygon());
	for (auto &feature : features) {
		if (result->addGeometryDirectly(feature.geom.get()) != OGRERR_NONE) {
			ThrowGDALError("Could not assemble the polygons of the raster");
		}
		feature.geom.release();
	}
	return OGRGeometryUniquePtr(result.release());
}

void CheckSameCRS(GDALDataset &a, GDALDataset &b, const char *function) {
	const auto crs_a = GetCRS(a);
	const auto crs_b = GetCRS(b);
	if (crs_a != crs_b && !crs_a.empty() && !crs_b.empty()) {
		throw InvalidInputException("%s: the rasters have different coordinate systems (%s and %s), see ST_Transform",
		                            function, crs_a, crs_b);
	}
}

LogicalType GeomValType() {
	child_list_t<LogicalType> children;
	children.emplace_back("geom", LogicalType::GEOMETRY());
	children.emplace_back("val", LogicalType::DOUBLE);
	return LogicalType::STRUCT(std::move(children));
}

Value GeomValList(const vector<Feature> &features) {
	vector<Value> result;
	for (const auto &feature : features) {
		child_list_t<Value> children;
		children.emplace_back("geom", GeometryValue(*feature.geom));
		children.emplace_back("val", Value::DOUBLE(feature.value));
		result.push_back(Value::STRUCT(std::move(children)));
	}
	return Value::LIST(GeomValType(), std::move(result));
}

//======================================================================================================================
// ST_DumpAsPolygons, ST_Polygon
//======================================================================================================================

// Gives a raster with the identity georeference an explicit one: GDAL's vector algorithms ignore an unset one
struct Georeferenced {
	GDALDatasetUniquePtr holder;
	GDALDataset *dataset;

	explicit Georeferenced(GDALDataset &src) : dataset(&src) {
		double gt[6];
		if (src.GetGeoTransform(gt) != CE_None) {
			holder = CopyToMem(src);
			GeoTransform().Apply(*holder);
			dataset = holder.get();
		}
	}
};

void DumpAsPolygonsExecute(Call &c) {
	const Georeferenced source(c.Raster());
	auto &band = GetBand(*source.dataset, c.Int("band", 1));
	c.ReturnValue(GeomValList(PolygonizeBand(band, c.Bool("exclude_nodata_value", true))));
}

void PolygonExecute(Call &c) {
	auto &ds = c.Raster();
	const auto band_number = c.Int("band", 1);
	GetBand(ds, band_number);
	const auto footprint = Footprint(ds, band_number);
	if (footprint) {
		c.ReturnGeometry(*footprint);
	}
}

//======================================================================================================================
// Spatial relationships
//======================================================================================================================

enum class Relation { INTERSECTS, CONTAINS, WITHIN, TOUCHES, OVERLAPS };

bool Relate(const OGRGeometry &a, const OGRGeometry &b, Relation relation) {
	CPLErrorReset();
	OGRBoolean result = FALSE;
	switch (relation) {
	case Relation::INTERSECTS:
		result = a.Intersects(&b);
		break;
	case Relation::CONTAINS:
		result = a.Contains(&b);
		break;
	case Relation::WITHIN:
		result = a.Within(&b);
		break;
	case Relation::TOUCHES:
		result = a.Touches(&b);
		break;
	case Relation::OVERLAPS:
		result = a.Overlaps(&b);
		break;
	}
	if (CPLGetLastErrorType() >= CE_Failure) {
		ThrowGDALError("Could not compare the geometries");
	}
	return result != FALSE;
}

template <Relation RELATION>
void RasterRelationExecute(Call &c) {
	auto &ds1 = c.Raster("rast1");
	auto &ds2 = c.Raster("rast2");
	CheckSameCRS(ds1, ds2, "Raster relationship");
	const auto footprint1 = Footprint(ds1, c.Int("nband1", 0));
	const auto footprint2 = Footprint(ds2, c.Int("nband2", 0));
	if (!footprint1 || !footprint2) {
		c.Return<bool>(false);
		return;
	}
	c.Return<bool>(Relate(*footprint1, *footprint2, RELATION));
}

void IntersectsGeometryExecute(Call &c) {
	auto &ds = c.Raster();
	const auto geom = c.Geometry("geom");
	const auto footprint = Footprint(ds, c.Int("nband", 0));
	if (!footprint || geom->IsEmpty()) {
		c.Return<bool>(false);
		return;
	}
	c.Return<bool>(Relate(*footprint, *geom, Relation::INTERSECTS));
}

//======================================================================================================================
// ST_Intersection
//======================================================================================================================

void IntersectionExecute(Call &c) {
	auto &ds = c.Raster();
	const auto geom = c.Geometry("geomin");
	const auto band_number = c.Int("band", 1);
	GetBand(ds, band_number);

	vector<Feature> result;
	if (!geom->IsEmpty()) {
		// Only the pixels under the bounding box of the geometry can contribute
		OGREnvelope envelope;
		geom->getEnvelope(&envelope);
		const GeoTransform gt(ds);
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
		const auto x0 = static_cast<int>(MaxValue<double>(0, std::floor(min_col)));
		const auto y0 = static_cast<int>(MaxValue<double>(0, std::floor(min_row)));
		const auto x1 = static_cast<int>(MinValue<double>(ds.GetRasterXSize(), std::ceil(max_col) + 1));
		const auto y1 = static_cast<int>(MinValue<double>(ds.GetRasterYSize(), std::ceil(max_row) + 1));
		if (x1 > x0 && y1 > y0) {
			const auto selected = SelectBands(ds, {band_number});
			const auto window = CopyWindow(*selected, x0, y0, x1 - x0, y1 - y0);
			// CopyWindow always sets a georeference
			for (auto &feature : PolygonizeBand(*window->GetRasterBand(1), true)) {
				CPLErrorReset();
				OGRGeometryUniquePtr part(feature.geom->Intersection(geom.get()));
				if (!part) {
					ThrowGDALError("ST_Intersection: could not intersect the geometry with the raster");
				}
				if (part->IsEmpty()) {
					continue;
				}
				feature.geom = std::move(part);
				result.push_back(std::move(feature));
			}
		}
	}
	c.ReturnValue(GeomValList(result));
}

//======================================================================================================================
// ST_AsRaster
//======================================================================================================================

void AsRasterExecute(Call &c) {
	const auto geom = c.Geometry("geom");
	if (geom->IsEmpty()) {
		return;
	}
	const auto type = ParsePixelType(c.String("pixeltype", "8BUI"));
	const auto value = c.Double("value", 1);
	const auto has_nodata = !c.Declares("nodataval") || c.Has("nodataval");
	const auto nodata = ClampToPixelType(c.Double("nodataval", 0), type);
	const auto touched = c.Bool("touched", false);

	OGREnvelope envelope;
	geom->getEnvelope(&envelope);

	Grid grid;
	string crs;
	if (c.Declares("ref")) {
		auto &ref = c.Raster("ref");
		const GeoTransform gt(ref);
		grid = CoverEnvelope(envelope, gt.c[1], gt.c[5], gt.c[2], gt.c[4], gt.c[0], gt.c[3]);
		crs = GetCRS(ref);
	} else {
		const auto extent_x = envelope.MaxX - envelope.MinX;
		const auto extent_y = envelope.MaxY - envelope.MinY;
		double scalex;
		double scaley;
		if (c.Declares("width")) {
			const auto width = c.Int("width");
			const auto height = c.Int("height");
			if (width <= 0 || height <= 0) {
				throw InvalidInputException("ST_AsRaster: the width and height must be positive");
			}
			scalex = extent_x / width;
			scaley = extent_y / height;
			// A geometry without extent in one direction gets square pixels
			if (scalex == 0) {
				scalex = scaley;
			}
			if (scaley == 0) {
				scaley = scalex;
			}
			if (scalex == 0) {
				scalex = scaley = 1;
			}
		} else {
			scalex = std::fabs(c.Double("scalex"));
			scaley = std::fabs(c.Double("scaley"));
			if (scalex == 0 || scaley == 0) {
				throw InvalidInputException("ST_AsRaster: the pixel size cannot be 0");
			}
		}
		// A given upper-left corner must be part of the raster; a grid point only aligns it
		if (c.Has("upperleftx") && c.Has("upperlefty")) {
			envelope.Merge(c.Double("upperleftx"), c.Double("upperlefty"));
		}
		const auto anchor_x = c.Has("upperleftx") ? c.Double("upperleftx") : c.Double("gridx", envelope.MinX);
		const auto anchor_y = c.Has("upperlefty") ? c.Double("upperlefty") : c.Double("gridy", envelope.MaxY);
		grid = CoverEnvelope(envelope, scalex, -scaley, c.Double("skewx", 0), c.Double("skewy", 0), anchor_x, anchor_y);
		if (c.Declares("width") && !c.Has("gridx") && !c.Has("upperleftx") && c.Double("skewx", 0) == 0 &&
		    c.Double("skewy", 0) == 0) {
			grid.width = c.Int("width");
			grid.height = c.Int("height");
		}
	}

	const auto result = CreateMemRaster(grid.width, grid.height, 1, type);
	grid.gt.Apply(*result);
	SetCRSText(*result, crs);
	auto &band = *result->GetRasterBand(1);
	if (has_nodata) {
		SetNoData(band, true, nodata);
		CheckGDAL(band.Fill(nodata), "Could not initialize the band");
	}
	BurnGeometry(*result, 1, *geom, value, touched);
	c.ReturnRaster(*result);
}

//======================================================================================================================
// ST_Contour
//======================================================================================================================

LogicalType ContourType() {
	child_list_t<LogicalType> children;
	children.emplace_back("geom", LogicalType::GEOMETRY());
	children.emplace_back("id", LogicalType::INTEGER);
	children.emplace_back("value", LogicalType::DOUBLE);
	return LogicalType::STRUCT(std::move(children));
}

void ContourExecute(Call &c) {
	const Georeferenced source(c.Raster());
	auto &band = GetBand(*source.dataset, c.Int("bandnumber", 1));
	const auto interval = c.Double("level_interval", 100.0);
	const auto polygonize = c.Bool("polygonize", false);
	vector<double> fixed_levels;
	if (c.Has("fixed_levels")) {
		fixed_levels = c.DoubleList("fixed_levels");
	}
	if (fixed_levels.empty() && !(interval > 0)) {
		throw InvalidInputException("ST_Contour: the level interval must be greater than 0");
	}

	CPLStringList options;
	if (!fixed_levels.empty()) {
		string levels;
		for (const auto level : fixed_levels) {
			levels += (levels.empty() ? "" : ",") + StringUtil::Format("%.17g", level);
		}
		options.SetNameValue("FIXED_LEVELS", levels.c_str());
	} else {
		options.SetNameValue("LEVEL_INTERVAL", StringUtil::Format("%.17g", interval).c_str());
		options.SetNameValue("LEVEL_BASE", StringUtil::Format("%.17g", c.Double("level_base", 0.0)).c_str());
	}
	double nodata;
	if (GetNoData(band, nodata)) {
		options.SetNameValue("NODATA", StringUtil::Format("%.17g", nodata).c_str());
	}
	options.SetNameValue("ID_FIELD", "0");
	if (polygonize) {
		options.SetNameValue("POLYGONIZE", "YES");
		options.SetNameValue("ELEV_FIELD_MIN", "1");
	} else {
		options.SetNameValue("ELEV_FIELD", "1");
	}

	FeatureSink sink;
	CPLErrorReset();
	CheckGDAL(GDALContourGenerateEx(GDALRasterBand::ToHandle(&band), sink.layer, options.List(), nullptr, nullptr),
	          "Could not compute the contours of the raster");

	vector<Value> result;
	for (const auto &feature : sink.Take()) {
		child_list_t<Value> children;
		children.emplace_back("geom", GeometryValue(*feature.geom));
		children.emplace_back("id", Value::INTEGER(NumericCast<int32_t>(feature.id)));
		children.emplace_back("value", Value::DOUBLE(feature.value));
		result.push_back(Value::STRUCT(std::move(children)));
	}
	c.ReturnValue(Value::LIST(ContourType(), std::move(result)));
}

//======================================================================================================================
// ST_InterpolateRaster
//======================================================================================================================

struct GridPoints {
	vector<double> x;
	vector<double> y;
	vector<double> z;

	void Add(const OGRGeometry &geom) {
		if (geom.IsEmpty()) {
			return;
		}
		switch (wkbFlatten(geom.getGeometryType())) {
		case wkbPoint: {
			const auto point = geom.toPoint();
			x.push_back(point->getX());
			y.push_back(point->getY());
			z.push_back(point->getZ());
			break;
		}
		case wkbLineString:
		case wkbCircularString: {
			const auto curve = geom.toSimpleCurve();
			for (int i = 0; i < curve->getNumPoints(); i++) {
				x.push_back(curve->getX(i));
				y.push_back(curve->getY(i));
				z.push_back(curve->getZ(i));
			}
			break;
		}
		case wkbPolygon: {
			const auto polygon = geom.toPolygon();
			for (const auto ring : *polygon) {
				Add(*ring);
			}
			break;
		}
		case wkbMultiPoint:
		case wkbMultiLineString:
		case wkbMultiPolygon:
		case wkbGeometryCollection: {
			const auto collection = geom.toGeometryCollection();
			for (const auto part : *collection) {
				Add(*part);
			}
			break;
		}
		default:
			throw InvalidInputException("ST_InterpolateRaster: unsupported geometry type %s", geom.getGeometryName());
		}
	}
};

struct CPLFreeDeleter {
	void operator()(void *ptr) const {
		CPLFree(ptr);
	}
};

void InterpolateRasterExecute(Call &c) {
	const auto geom = c.Geometry("geom");
	if (!geom->Is3D()) {
		throw InvalidInputException("ST_InterpolateRaster: the geometry must have Z values to interpolate");
	}
	GridPoints points;
	points.Add(*geom);
	if (points.x.empty()) {
		throw InvalidInputException("ST_InterpolateRaster: the geometry has no points");
	}

	auto &src = c.Raster();
	const auto band_number = c.Int("bandnumber", 1);
	auto &src_band = GetBand(src, band_number);
	const GeoTransform gt(src);
	if (gt.c[2] != 0 || gt.c[4] != 0) {
		throw InvalidInputException("ST_InterpolateRaster: the raster must not be skewed");
	}
	const auto width = src.GetRasterXSize();
	const auto height = src.GetRasterYSize();

	GDALGridAlgorithm algorithm;
	void *raw_options = nullptr;
	CPLErrorReset();
	const auto options_text = c.String("options");
	if (GDALGridParseAlgorithmAndOptions(options_text.c_str(), &algorithm, &raw_options) != CE_None || !raw_options) {
		CPLFree(raw_options);
		ThrowGDALError(StringUtil::Format("ST_InterpolateRaster: invalid interpolation options '%s', expected a "
		                                  "gdal_grid algorithm such as 'invdist:power:2.0' or 'nearest'",
		                                  options_text));
	}
	const std::unique_ptr<void, CPLFreeDeleter> options(raw_options);

	// The first row GDAL computes is at the first Y coordinate it is given: pass the top edge to get raster order
	vector<double> values(static_cast<idx_t>(width) * height);
	CPLErrorReset();
	CheckGDAL(GDALGridCreate(algorithm, options.get(), NumericCast<GUInt32>(points.x.size()), points.x.data(),
	                         points.y.data(), points.z.data(), gt.c[0], gt.c[0] + width * gt.c[1], gt.c[3],
	                         gt.c[3] + height * gt.c[5], NumericCast<GUInt32>(width), NumericCast<GUInt32>(height),
	                         GDT_Float64, values.data(), nullptr, nullptr),
	          "ST_InterpolateRaster: could not interpolate the points");

	const auto result = CopyToMem(src);
	auto &band = *result->GetRasterBand(band_number);
	// Algorithms write their own NODATA value (0 by default) where they have no result
	double nodata;
	if (!GetNoData(src_band, nodata)) {
		SetNoData(band, false, 0);
	}
	WriteBand(band, values);
	c.ReturnRaster(*result);
}

} // namespace

//======================================================================================================================
// Registration
//======================================================================================================================

void RegisterRasterVectorFunctions(ExtensionLoader &loader) {
	const auto RASTER = RasterType();
	const auto BOOL = LogicalType::BOOLEAN;
	const auto GEOM = LogicalType::GEOMETRY();
	const auto geomvals = LogicalType::LIST(GeomValType());

	RasterFunction("ST_AsRaster")
	    .AddOptional({GeomP(), RastP("ref")}, {TextP("pixeltype"), DblP("value"), DblP("nodataval", true), BoolP("touched")},
	                 RASTER, AsRasterExecute)
	    .AddOptional({GeomP(), DblP("scalex"), DblP("scaley"), TextP("pixeltype")},
	                 {DblP("value"), DblP("nodataval", true), DblP("upperleftx", true), DblP("upperlefty", true),
	                  DblP("skewx"), DblP("skewy"), BoolP("touched")},
	                 RASTER, AsRasterExecute)
	    .AddOptional({GeomP(), DblP("scalex"), DblP("scaley"), DblP("gridx"), DblP("gridy"), TextP("pixeltype")},
	                 {DblP("value"), DblP("nodataval", true), DblP("skewx"), DblP("skewy"), BoolP("touched")}, RASTER,
	                 AsRasterExecute)
	    .AddOptional({GeomP(), IntP("width"), IntP("height"), TextP("pixeltype")},
	                 {DblP("value"), DblP("nodataval", true), DblP("upperleftx", true), DblP("upperlefty", true),
	                  DblP("skewx"), DblP("skewy"), BoolP("touched")},
	                 RASTER, AsRasterExecute)
	    .AddOptional({GeomP(), IntP("width"), IntP("height"), DblP("gridx"), DblP("gridy"), TextP("pixeltype")},
	                 {DblP("value"), DblP("nodataval", true), DblP("skewx"), DblP("skewy"), BoolP("touched")}, RASTER,
	                 AsRasterExecute)
	    .Describe(R"(
		Rasterizes a geometry: returns a single-band raster that covers the bounding box of the geometry, where the pixels covered by the geometry have `value` (default 1) and the others are NODATA.

		The grid is that of a reference raster `ref` (same pixel size, skew, alignment and coordinate system), or is given by a pixel size (`scalex`, `scaley`, in world units, sign ignored) or a size in pixels (`width`, `height`). `upperleftx`/`upperlefty` fix the upper-left corner of the result, `gridx`/`gridy` only align its pixel corners with that point; `skewx`/`skewy` default to 0. Without a reference raster the result is north-up and has no coordinate system.

		`pixeltype` defaults to `8BUI` and `nodataval` to 0; a NULL `nodataval` gives a band without NODATA value whose background is 0. A polygon covers the pixels whose centre it contains, lines and points the pixels they pass through; with `touched` every pixel the geometry touches is covered. Returns NULL for an empty geometry. The variants of PostGIS that take lists of pixel types, values and NODATA values (several bands) are not available.
	)",
	              R"(
		SELECT ST_DumpValues(ST_AsRaster(ST_MakeEnvelope(0, 0, 2, 1), 1.0, 1.0, '8BUI', 7));
		----
		[[7.0, 7.0]]
	)")
	    .Register(loader);

	RasterFunction("ST_DumpAsPolygons")
	    .AddOptional({RastP()}, {IntP("band"), BoolP("exclude_nodata_value")}, geomvals, DumpAsPolygonsExecute)
	    .Describe(R"(
		Vectorizes a band: returns a list of structs `(geom, val)` with one polygon per connected area of pixels that share a value. PostGIS returns a set of rows: use `UNNEST(..., recursive := true)` to get the same shape.

		`band` is 1-based and defaults to 1. NODATA pixels are left out unless `exclude_nodata_value` is false. Pixels that only touch at a corner are not connected. Bands of type `32BUI`, `32BF` and `64BF` are compared as 32-bit floats.
	)",
	              R"(
		SELECT UNNEST(ST_DumpAsPolygons(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(2, 1, 0, 1, 1), '8BUI', 1), 2, 1, 5)), recursive := true);
		----
		POLYGON ((0 1, 0 0, 1 0, 1 1, 0 1))	1.0
		POLYGON ((1 1, 1 0, 2 0, 2 1, 1 1))	5.0
	)")
	    .Register(loader);

	RasterFunction("ST_Polygon")
	    .AddOptional({RastP()}, {IntP("band")}, GEOM, PolygonExecute)
	    .Describe(R"(
		Returns the area covered by the pixels of a band (1-based, default 1) that are not NODATA, as a polygon or a multipolygon. Returns NULL if all pixels are NODATA.
	)",
	              R"(
		SELECT ST_Polygon(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(3, 1, 0, 1, 1), '8BUI', 1, 0), 3, 1, NULL));
		----
		POLYGON ((0 1, 0 0, 2 0, 2 1, 0 1))
	)")
	    .Register(loader);

	RasterFunction("ST_Intersects")
	    .AddOptional({RastP(), GeomP()}, {IntP("nband", true)}, BOOL, IntersectsGeometryExecute)
	    .AddOptional({GeomP(), RastP()}, {IntP("nband", true)}, BOOL, IntersectsGeometryExecute)
	    .Add({RastP(), IntP("nband"), GeomP()}, BOOL, IntersectsGeometryExecute)
	    .Add({RastP("rast1"), RastP("rast2")}, BOOL, RasterRelationExecute<Relation::INTERSECTS>)
	    .Add({RastP("rast1"), IntP("nband1"), RastP("rast2"), IntP("nband2")}, BOOL,
	         RasterRelationExecute<Relation::INTERSECTS>)
	    .Describe(R"(
		Returns true if a raster intersects a geometry or another raster.

		Without band numbers the outline of the raster (see ST_ConvexHull) is tested. With a band number (1-based) the area of the pixels of that band that are not NODATA is tested (see ST_Polygon), so a geometry that only crosses NODATA pixels does not intersect. The geometry must be in the coordinate system of the raster; two rasters must have the same coordinate system but need not be aligned.
	)",
	              R"(
		SELECT ST_Intersects(ST_AddBand(ST_MakeEmptyRaster(4, 4, 0, 4, 1), '8BUI', 1), ST_Point(1.5, 1.5));
		----
		true
	)")
	    .Register(loader);

	RasterFunction("ST_Intersection")
	    .Add({RastP(), GeomP("geomin")}, geomvals, IntersectionExecute)
	    .Add({RastP(), IntP("band"), GeomP("geomin")}, geomvals, IntersectionExecute)
	    .AddOptional({GeomP("geomin"), RastP()}, {IntP("band")}, geomvals, IntersectionExecute)
	    .Describe(R"(
		Returns the parts of a geometry that lie over each area of equal pixel values of a band, as a list of structs `(geom, val)`. PostGIS returns a set of rows: use `UNNEST(..., recursive := true)` to get the same shape.

		The band (1-based, default 1) is vectorized as by ST_DumpAsPolygons and each polygon is intersected with the geometry; NODATA pixels do not contribute. The geometry must be in the coordinate system of the raster. The variants of PostGIS that intersect two rasters and return a raster are not available: use the two-raster form of ST_MapAlgebra.
	)",
	              R"(
		SELECT UNNEST(ST_Intersection(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(2, 1, 0, 1, 1), '8BUI', 1), 2, 1, 5), ST_MakeEnvelope(0.5, 0, 1.5, 0.5)), recursive := true);
		----
		POLYGON ((0.5 0, 0.5 0.5, 1 0.5, 1 0, 0.5 0))	1.0
		POLYGON ((1 0.5, 1.5 0.5, 1.5 0, 1 0, 1 0.5))	5.0
	)")
	    .Register(loader);

	const char *relation_description = R"(
		Returns true if the first raster %s the second one.

		Without band numbers the outlines of the rasters (see ST_ConvexHull) are compared; with band numbers (1-based), the areas of the pixels of those bands that are not NODATA (see ST_Polygon). The rasters must have the same coordinate system but need not be aligned.
	)";
	const auto contains_description = StringUtil::Format(relation_description, "contains");
	const auto within_description = StringUtil::Format(relation_description, "lies within");
	const auto touches_description =
	    StringUtil::Format(relation_description, "touches (shares boundary points but no interior points with)");
	const auto overlaps_description =
	    StringUtil::Format(relation_description, "overlaps (shares some but not all of its area with)");

	RasterFunction("ST_Contains")
	    .Add({RastP("rast1"), RastP("rast2")}, BOOL, RasterRelationExecute<Relation::CONTAINS>)
	    .Add({RastP("rast1"), IntP("nband1"), RastP("rast2"), IntP("nband2")}, BOOL,
	         RasterRelationExecute<Relation::CONTAINS>)
	    .Describe(contains_description.c_str(),
	              R"(
		SELECT ST_Contains(ST_MakeEmptyRaster(4, 4, 0, 4, 1), ST_MakeEmptyRaster(2, 2, 1, 3, 1));
		----
		true
	)")
	    .Register(loader);

	RasterFunction("ST_Within")
	    .Add({RastP("rast1"), RastP("rast2")}, BOOL, RasterRelationExecute<Relation::WITHIN>)
	    .Add({RastP("rast1"), IntP("nband1"), RastP("rast2"), IntP("nband2")}, BOOL,
	         RasterRelationExecute<Relation::WITHIN>)
	    .Describe(within_description.c_str(),
	              R"(
		SELECT ST_Within(ST_MakeEmptyRaster(2, 2, 1, 3, 1), ST_MakeEmptyRaster(4, 4, 0, 4, 1));
		----
		true
	)")
	    .Register(loader);

	RasterFunction("ST_Touches")
	    .Add({RastP("rast1"), RastP("rast2")}, BOOL, RasterRelationExecute<Relation::TOUCHES>)
	    .Add({RastP("rast1"), IntP("nband1"), RastP("rast2"), IntP("nband2")}, BOOL,
	         RasterRelationExecute<Relation::TOUCHES>)
	    .Describe(touches_description.c_str(),
	              R"(
		SELECT ST_Touches(ST_MakeEmptyRaster(2, 2, 0, 2, 1), ST_MakeEmptyRaster(2, 2, 2, 2, 1));
		----
		true
	)")
	    .Register(loader);

	RasterFunction("ST_Overlaps")
	    .Add({RastP("rast1"), RastP("rast2")}, BOOL, RasterRelationExecute<Relation::OVERLAPS>)
	    .Add({RastP("rast1"), IntP("nband1"), RastP("rast2"), IntP("nband2")}, BOOL,
	         RasterRelationExecute<Relation::OVERLAPS>)
	    .Describe(overlaps_description.c_str(),
	              R"(
		SELECT ST_Overlaps(ST_MakeEmptyRaster(2, 2, 0, 2, 1), ST_MakeEmptyRaster(2, 2, 1, 2, 1));
		----
		true
	)")
	    .Register(loader);

	const auto contours = LogicalType::LIST(ContourType());
	RasterFunction("ST_Contour")
	    .AddOptional({RastP()},
	                 {IntP("bandnumber"), DblP("level_interval"), DblP("level_base"),
	                  Param("fixed_levels", LogicalType::LIST(LogicalType::DOUBLE), true), BoolP("polygonize")},
	                 contours, ContourExecute)
	    .Describe(R"(
		Returns the contour lines of a band as a list of structs `(geom, id, value)`. PostGIS returns a set of rows: use `UNNEST(..., recursive := true)` to get the same shape.

		Contours are drawn every `level_interval` (default 100) starting from `level_base` (default 0), or at the `fixed_levels` if that list is not empty. `bandnumber` is 1-based and defaults to 1; NODATA pixels are ignored. With `polygonize` the result is polygons of the areas between consecutive levels instead of lines, and `value` is the lower bound that GDAL reports for each area. Pixel values are taken at pixel centres; in the outer half pixel of the raster the surface is the value of the outermost pixel centres.
	)",
	              R"(
		SELECT c.id, c.value, ST_GeometryType(c.geom) FROM (SELECT UNNEST(ST_Contour(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(3, 3, 0, 3, 1), '32BF', 0), 2, 2, 10), 1, 100.0, 0.0, [5.0])) AS c);
		----
		0	5.0	LINESTRING
	)")
	    .Register(loader);

	RasterFunction("ST_InterpolateRaster")
	    .AddOptional({GeomP(), TextP("options"), RastP()}, {IntP("bandnumber")}, RASTER, InterpolateRasterExecute)
	    .Describe(R"(
		Interpolates a surface from the 3D points of a geometry onto the grid of a raster, and returns the raster with band `bandnumber` (1-based, default 1) replaced by the interpolated values.

		Every vertex of the geometry, which must have Z values, is an input point. `options` names the algorithm and its parameters in the syntax of `gdal_grid`: `invdist[:power:2.0:smoothing:0.0:...]`, `invdistnn`, `average`, `nearest` and the other GDAL gridding algorithms (`linear` needs a GDAL built with QHull, which the bundled one is not). Values are computed at pixel centres. The raster must not be skewed; the geometry must be in its coordinate system.
	)",
	              R"(
		SELECT ST_DumpValues(ST_InterpolateRaster(ST_GeomFromText('MULTIPOINT Z ((0.5 0.5 10), (1.5 0.5 20))'), 'nearest', ST_AddBand(ST_MakeEmptyRaster(2, 1, 0, 1, 1), '32BF')));
		----
		[[10.0, 20.0]]
	)")
	    .Register(loader);
}

} // namespace raster
} // namespace duckdb
