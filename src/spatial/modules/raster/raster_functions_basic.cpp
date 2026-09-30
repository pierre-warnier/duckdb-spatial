#include "spatial/modules/raster/raster_core.hpp"

namespace duckdb {
namespace raster {

namespace {

//======================================================================================================================
// Constructors and bands
//======================================================================================================================

void MakeEmptyRasterExecute(Call &c) {
	if (!c.Declares("width")) {
		const auto ds = CreateMemLike(c.Raster(), 0, GDT_Byte);
		c.ReturnRaster(*ds);
		return;
	}
	const auto ds = CreateMemRaster(c.Int("width"), c.Int("height"), 0, GDT_Byte);
	GeoTransform gt;
	gt.c[0] = c.Double("upperleftx");
	gt.c[3] = c.Double("upperlefty");
	if (c.Declares("pixelsize")) {
		gt.c[1] = c.Double("pixelsize");
		gt.c[5] = -c.Double("pixelsize");
	} else {
		gt.c[1] = c.Double("scalex");
		gt.c[5] = c.Double("scaley");
		gt.c[2] = c.Double("skewx");
		gt.c[4] = c.Double("skewy");
	}
	gt.Apply(*ds);
	SetSRID(*ds, c.Int("srid", 0));
	c.ReturnRaster(*ds);
}

// Moves the last `moved` bands so that the first of them becomes band `index`
GDALDatasetUniquePtr MoveNewBands(GDALDatasetUniquePtr ds, int32_t moved, int32_t index) {
	const auto total = ds->GetRasterCount();
	const auto kept = total - moved;
	if (index < 1) {
		throw InvalidInputException("Band index must be at least 1, got %d", index);
	}
	if (index > kept) {
		return ds;
	}
	vector<int32_t> order;
	for (int32_t i = 1; i < index; i++) {
		order.push_back(i);
	}
	for (int32_t i = kept + 1; i <= total; i++) {
		order.push_back(i);
	}
	for (int32_t i = index; i <= kept; i++) {
		order.push_back(i);
	}
	return SelectBands(*ds, order);
}

void AddBandExecute(Call &c) {
	auto ds = c.RasterCopy();
	AddBand(*ds, ParsePixelType(c.String("pixeltype")));
	auto &band = *ds->GetRasterBand(ds->GetRasterCount());
	const auto initial = c.Double("initialvalue", 0);
	if (initial != 0) {
		CheckGDAL(band.Fill(initial), "Could not initialize the band");
	}
	if (c.Has("nodataval")) {
		SetNoData(band, true, c.Double("nodataval"));
	}
	if (c.Declares("index")) {
		ds = MoveNewBands(std::move(ds), 1, c.Int("index"));
	}
	c.ReturnRaster(*ds);
}

void AppendBand(GDALDataset &target, GDALDataset &source, int32_t band_number) {
	auto &src_band = GetBand(source, band_number);
	AddBand(target, src_band.GetRasterDataType());
	CopyBand(src_band, *target.GetRasterBand(target.GetRasterCount()));
}

void AddBandFromRasterExecute(Call &c) {
	auto ds = c.RasterCopy("torast");
	const auto from_band = c.Int("fromband", 1);
	int32_t added = 0;
	if (c.Declares("fromrast")) {
		AppendBand(*ds, c.Raster("fromrast"), from_band);
		added = 1;
	} else {
		const auto list = c.GetValue("fromrasts");
		for (const auto &child : ListValue::GetChildren(list)) {
			if (child.IsNull()) {
				throw InvalidInputException("ST_AddBand: NULL is not allowed in the list of rasters");
			}
			const auto source = OpenRaster(StringValue::Get(child));
			AppendBand(*ds, *source.dataset, from_band);
			added++;
		}
	}
	if (c.Has("torastindex")) {
		ds = MoveNewBands(std::move(ds), added, c.Int("torastindex"));
	}
	c.ReturnRaster(*ds);
}

void BandExecute(Call &c) {
	vector<int32_t> bands;
	if (c.Declares("nbands")) {
		bands = c.IntList("nbands");
	} else {
		bands.push_back(c.Int("nband", 1));
	}
	const auto ds = SelectBands(c.Raster(), bands);
	c.ReturnRaster(*ds);
}

//======================================================================================================================
// Accessors
//======================================================================================================================

void NumBandsExecute(Call &c) {
	c.Return<int32_t>(c.Raster().GetRasterCount());
}

void WidthExecute(Call &c) {
	c.Return<int32_t>(c.Raster().GetRasterXSize());
}

void HeightExecute(Call &c) {
	c.Return<int32_t>(c.Raster().GetRasterYSize());
}

template <int INDEX>
void GeoTransformTermExecute(Call &c) {
	c.Return<double>(GeoTransform(c.Raster()).c[INDEX]);
}

void PixelWidthExecute(Call &c) {
	const GeoTransform gt(c.Raster());
	c.Return<double>(std::sqrt(gt.c[1] * gt.c[1] + gt.c[4] * gt.c[4]));
}

void PixelHeightExecute(Call &c) {
	const GeoTransform gt(c.Raster());
	c.Return<double>(std::sqrt(gt.c[5] * gt.c[5] + gt.c[2] * gt.c[2]));
}

void RotationExecute(Call &c) {
	const GeoTransform gt(c.Raster());
	const auto magnitude = std::sqrt(gt.c[1] * gt.c[1] + gt.c[4] * gt.c[4]);
	auto theta = std::acos(gt.c[1] / magnitude);
	if (std::acos(gt.c[4] / magnitude) < M_PI / 2) {
		theta = -theta;
	}
	c.Return<double>(theta);
}

bool IsESRIFormat(const string &format) {
	if (StringUtil::CIEquals(format, "ESRI")) {
		return true;
	}
	if (StringUtil::CIEquals(format, "GDAL")) {
		return false;
	}
	throw InvalidInputException("Unknown georeference format '%s', expected 'GDAL' or 'ESRI'", format);
}

void GeoReferenceExecute(Call &c) {
	const GeoTransform gt(c.Raster());
	auto x = gt.c[0];
	auto y = gt.c[3];
	if (IsESRIFormat(c.String("format", "GDAL"))) {
		x += gt.c[1] * 0.5;
		y += gt.c[5] * 0.5;
	}
	c.ReturnString(
	    StringUtil::Format("%.10f\n%.10f\n%.10f\n%.10f\n%.10f\n%.10f\n", gt.c[1], gt.c[4], gt.c[2], gt.c[5], x, y));
}

LogicalType MetaDataType() {
	child_list_t<LogicalType> children;
	children.emplace_back("upperleftx", LogicalType::DOUBLE);
	children.emplace_back("upperlefty", LogicalType::DOUBLE);
	children.emplace_back("width", LogicalType::INTEGER);
	children.emplace_back("height", LogicalType::INTEGER);
	children.emplace_back("scalex", LogicalType::DOUBLE);
	children.emplace_back("scaley", LogicalType::DOUBLE);
	children.emplace_back("skewx", LogicalType::DOUBLE);
	children.emplace_back("skewy", LogicalType::DOUBLE);
	children.emplace_back("srid", LogicalType::INTEGER);
	children.emplace_back("numbands", LogicalType::INTEGER);
	return LogicalType::STRUCT(std::move(children));
}

void MetaDataExecute(Call &c) {
	auto &ds = c.Raster();
	const GeoTransform gt(ds);
	child_list_t<Value> children;
	children.emplace_back("upperleftx", Value::DOUBLE(gt.c[0]));
	children.emplace_back("upperlefty", Value::DOUBLE(gt.c[3]));
	children.emplace_back("width", Value::INTEGER(ds.GetRasterXSize()));
	children.emplace_back("height", Value::INTEGER(ds.GetRasterYSize()));
	children.emplace_back("scalex", Value::DOUBLE(gt.c[1]));
	children.emplace_back("scaley", Value::DOUBLE(gt.c[5]));
	children.emplace_back("skewx", Value::DOUBLE(gt.c[2]));
	children.emplace_back("skewy", Value::DOUBLE(gt.c[4]));
	children.emplace_back("srid", Value::INTEGER(GetSRID(ds)));
	children.emplace_back("numbands", Value::INTEGER(ds.GetRasterCount()));
	c.ReturnValue(Value::STRUCT(std::move(children)));
}

LogicalType BandMetaDataType() {
	child_list_t<LogicalType> children;
	children.emplace_back("pixeltype", LogicalType::VARCHAR);
	children.emplace_back("nodatavalue", LogicalType::DOUBLE);
	children.emplace_back("isoutdb", LogicalType::BOOLEAN);
	children.emplace_back("path", LogicalType::VARCHAR);
	return LogicalType::STRUCT(std::move(children));
}

void BandMetaDataExecute(Call &c) {
	auto &band = c.Band();
	double nodata;
	const auto has_nodata = GetNoData(band, nodata);
	child_list_t<Value> children;
	children.emplace_back("pixeltype", Value(PixelTypeName(band.GetRasterDataType())));
	children.emplace_back("nodatavalue", has_nodata ? Value::DOUBLE(nodata) : Value(LogicalType::DOUBLE));
	children.emplace_back("isoutdb", Value::BOOLEAN(false));
	children.emplace_back("path", Value(LogicalType::VARCHAR));
	c.ReturnValue(Value::STRUCT(std::move(children)));
}

void BandPixelTypeExecute(Call &c) {
	c.ReturnString(PixelTypeName(c.Band().GetRasterDataType()));
}

void BandNoDataValueExecute(Call &c) {
	double nodata;
	if (GetNoData(c.Band(), nodata)) {
		c.Return<double>(nodata);
	}
}

void HasNoBandExecute(Call &c) {
	const auto band = c.Int("band", 1);
	c.Return<bool>(band < 1 || band > c.Raster().GetRasterCount());
}

void BandIsNoDataExecute(Call &c) {
	const BandValues band(c.Band(), true);
	auto all_nodata = band.has_nodata;
	for (idx_t i = 0; all_nodata && i < band.values.size(); i++) {
		all_nodata = !band.IsValid(i);
	}
	c.Return<bool>(all_nodata);
}

void SRIDExecute(Call &c) {
	c.Return<int32_t>(GetSRID(c.Raster()));
}

void CRSExecute(Call &c) {
	const auto crs = GetCRS(c.Raster());
	if (!crs.empty()) {
		c.ReturnString(crs);
	}
}

void EnvelopeExecute(Call &c) {
	auto &ds = c.Raster();
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
	const double xs[4] = {envelope.MinX, envelope.MaxX, envelope.MaxX, envelope.MinX};
	const double ys[4] = {envelope.MinY, envelope.MinY, envelope.MaxY, envelope.MaxY};
	c.ReturnGeometry(*MakePolygon(xs, ys, 4));
}

void ConvexHullExecute(Call &c) {
	auto &ds = c.Raster();
	c.ReturnGeometry(*PixelPolygon(GeoTransform(ds), 0, 0, ds.GetRasterXSize(), ds.GetRasterYSize()));
}

void MinConvexHullExecute(Call &c) {
	auto &ds = c.Raster();
	const auto width = ds.GetRasterXSize();
	const auto height = ds.GetRasterYSize();
	int first_band = 1;
	int last_band = ds.GetRasterCount();
	if (c.Has("nband")) {
		GetBand(ds, c.Int("nband"));
		first_band = last_band = c.Int("nband");
	}

	int min_x = width;
	int min_y = height;
	int max_x = -1;
	int max_y = -1;
	for (int b = first_band; b <= last_band; b++) {
		const BandValues band(*ds.GetRasterBand(b), true);
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				if (band.IsValid(static_cast<idx_t>(y) * width + x)) {
					min_x = MinValue(min_x, x);
					max_x = MaxValue(max_x, x);
					min_y = MinValue(min_y, y);
					max_y = MaxValue(max_y, y);
				}
			}
		}
	}
	if (max_x < 0) {
		return;
	}
	c.ReturnGeometry(*PixelPolygon(GeoTransform(ds), min_x, min_y, max_x + 1, max_y + 1));
}

//======================================================================================================================
// Coordinates
//======================================================================================================================

// Returns false for an empty point
bool PointToWorld(const OGRGeometry &geom, double &x, double &y) {
	if (wkbFlatten(geom.getGeometryType()) != wkbPoint) {
		throw InvalidInputException("The geometry must be a POINT, got %s", geom.getGeometryName());
	}
	const auto point = geom.toPoint();
	if (point->IsEmpty() || std::isnan(point->getX())) {
		return false;
	}
	x = point->getX();
	y = point->getY();
	return true;
}

bool PointToPixel(GDALDataset &ds, const OGRGeometry &geom, int64_t &col, int64_t &row) {
	double x;
	double y;
	if (!PointToWorld(geom, x, y)) {
		return false;
	}
	double pixel_col;
	double pixel_row;
	GeoTransform(ds).ToPixel(x, y, pixel_col, pixel_row);
	col = PixelFloor(pixel_col);
	row = PixelFloor(pixel_row);
	return true;
}

LogicalType WorldCoordType() {
	child_list_t<LogicalType> children;
	children.emplace_back("longitude", LogicalType::DOUBLE);
	children.emplace_back("latitude", LogicalType::DOUBLE);
	return LogicalType::STRUCT(std::move(children));
}

LogicalType RasterCoordType() {
	child_list_t<LogicalType> children;
	children.emplace_back("columnx", LogicalType::INTEGER);
	children.emplace_back("rowy", LogicalType::INTEGER);
	return LogicalType::STRUCT(std::move(children));
}

void RasterToWorldCoordExecute(Call &c) {
	double x;
	double y;
	GeoTransform(c.Raster()).ToWorld(c.Int("columnx") - 1, c.Int("rowy") - 1, x, y);
	child_list_t<Value> children;
	children.emplace_back("longitude", Value::DOUBLE(x));
	children.emplace_back("latitude", Value::DOUBLE(y));
	c.ReturnValue(Value::STRUCT(std::move(children)));
}

template <bool IS_X>
void RasterToWorldCoordAxisExecute(Call &c) {
	const GeoTransform gt(c.Raster());
	if (!c.Declares(IS_X ? "yr" : "xr") && (gt.c[2] != 0 || gt.c[4] != 0)) {
		throw InvalidInputException("The raster is skewed: both the column and the row are needed to compute a world "
		                            "coordinate");
	}
	double x;
	double y;
	gt.ToWorld(c.Int("xr", 1) - 1, c.Int("yr", 1) - 1, x, y);
	c.Return<double>(IS_X ? x : y);
}

bool WorldArguments(Call &c, double &x, double &y) {
	if (c.Declares("pt")) {
		return PointToWorld(*c.Geometry("pt"), x, y);
	}
	const GeoTransform gt(c.Raster());
	if ((!c.Declares("xw") || !c.Declares("yw")) && (gt.c[2] != 0 || gt.c[4] != 0)) {
		throw InvalidInputException("The raster is skewed: both world coordinates are needed to compute a pixel "
		                            "coordinate");
	}
	x = c.Double("xw", gt.c[0]);
	y = c.Double("yw", gt.c[3]);
	return true;
}

int32_t PixelNumber(int64_t zero_based) {
	const auto one_based = zero_based + 1;
	if (one_based < NumericLimits<int32_t>::Minimum() || one_based > NumericLimits<int32_t>::Maximum()) {
		throw InvalidInputException("The pixel coordinate is out of the INTEGER range");
	}
	return static_cast<int32_t>(one_based);
}

void WorldToRasterCoordExecute(Call &c) {
	double x;
	double y;
	if (!WorldArguments(c, x, y)) {
		return;
	}
	double col;
	double row;
	GeoTransform(c.Raster()).ToPixel(x, y, col, row);
	child_list_t<Value> children;
	children.emplace_back("columnx", Value::INTEGER(PixelNumber(PixelFloor(col))));
	children.emplace_back("rowy", Value::INTEGER(PixelNumber(PixelFloor(row))));
	c.ReturnValue(Value::STRUCT(std::move(children)));
}

template <bool IS_X>
void WorldToRasterCoordAxisExecute(Call &c) {
	double x;
	double y;
	if (!WorldArguments(c, x, y)) {
		return;
	}
	double col;
	double row;
	GeoTransform(c.Raster()).ToPixel(x, y, col, row);
	c.Return<int32_t>(PixelNumber(PixelFloor(IS_X ? col : row)));
}

//======================================================================================================================
// Pixels
//======================================================================================================================

// 0-based pixel of the call, from (x, y) or from a point. Returns false for an empty point
bool PixelArguments(Call &c, GDALDataset &ds, int64_t &col, int64_t &row) {
	if (c.Declares("pt")) {
		return PointToPixel(ds, *c.Geometry("pt"), col, row);
	}
	col = static_cast<int64_t>(c.Int("x")) - 1;
	row = static_cast<int64_t>(c.Int("y")) - 1;
	return true;
}

bool InRaster(GDALDataset &ds, int64_t col, int64_t row) {
	return col >= 0 && row >= 0 && col < ds.GetRasterXSize() && row < ds.GetRasterYSize();
}

void ValueExecute(Call &c) {
	auto &ds = c.Raster();
	auto &band = c.Band();
	int64_t col;
	int64_t row;
	if (!PixelArguments(c, ds, col, row) || !InRaster(ds, col, row)) {
		return;
	}
	double value;
	ReadPixels(band, static_cast<int>(col), static_cast<int>(row), 1, 1, &value);
	double nodata;
	if (c.Bool("exclude_nodata_value", true) && GetNoData(band, nodata) && IsNoData(value, true, nodata)) {
		return;
	}
	c.Return<double>(value);
}

void SetValueExecute(Call &c) {
	const auto ds = c.RasterCopy();
	const auto band_number = c.Int("band", 1);
	auto &band = GetBand(*ds, band_number);

	double value;
	if (c.Has("newvalue")) {
		value = c.Double("newvalue");
	} else if (!GetNoData(band, value)) {
		throw InvalidInputException("ST_SetValue: cannot set a pixel to NULL, band %d has no NODATA value",
		                            band_number);
	}

	if (c.Declares("geom")) {
		const auto geom = c.Geometry("geom");
		if (!geom->IsEmpty()) {
			BurnGeometry(*ds, band_number, *geom, value, false);
		}
	} else {
		const auto col = static_cast<int64_t>(c.Int("x")) - 1;
		const auto row = static_cast<int64_t>(c.Int("y")) - 1;
		if (!InRaster(*ds, col, row)) {
			throw InvalidInputException("ST_SetValue: pixel (%d, %d) is outside of the %d x %d raster (pixels are "
			                            "numbered from 1)",
			                            c.Int("x"), c.Int("y"), ds->GetRasterXSize(), ds->GetRasterYSize());
		}
		WritePixels(band, static_cast<int>(col), static_cast<int>(row), 1, 1, &value);
	}
	c.ReturnRaster(*ds);
}

void NearestValueExecute(Call &c) {
	auto &ds = c.Raster();
	const GeoTransform gt(ds);
	const BandValues band(c.Band(), c.Bool("exclude_nodata_value", true));

	double x;
	double y;
	if (c.Declares("pt")) {
		if (!PointToWorld(*c.Geometry("pt"), x, y)) {
			return;
		}
	} else {
		gt.ToWorld(c.Int("columnx") - 0.5, c.Int("rowy") - 0.5, x, y);
	}

	double col;
	double row;
	gt.ToPixel(x, y, col, row);
	const auto pixel_col = PixelFloor(col);
	const auto pixel_row = PixelFloor(row);
	if (InRaster(ds, pixel_col, pixel_row)) {
		const auto idx = static_cast<idx_t>(pixel_row) * band.width + static_cast<idx_t>(pixel_col);
		if (band.IsValid(idx)) {
			c.Return<double>(band.values[idx]);
			return;
		}
	}

	auto best_distance = std::numeric_limits<double>::infinity();
	auto found = false;
	double best_value = 0;
	for (int py = 0; py < band.height; py++) {
		for (int px = 0; px < band.width; px++) {
			const auto idx = static_cast<idx_t>(py) * band.width + px;
			if (!band.IsValid(idx)) {
				continue;
			}
			double cx;
			double cy;
			gt.ToWorld(px + 0.5, py + 0.5, cx, cy);
			const auto distance = (cx - x) * (cx - x) + (cy - y) * (cy - y);
			if (distance < best_distance) {
				best_distance = distance;
				best_value = band.values[idx];
				found = true;
			}
		}
	}
	if (found) {
		c.Return<double>(best_value);
	}
}

enum class PixelShape { POINT, CENTROID, POLYGON };

OGRGeometryUniquePtr PixelGeometry(const GeoTransform &gt, PixelShape shape, int64_t col, int64_t row) {
	if (shape == PixelShape::POLYGON) {
		return PixelPolygon(gt, static_cast<double>(col), static_cast<double>(row), static_cast<double>(col + 1),
		                    static_cast<double>(row + 1));
	}
	const auto offset = shape == PixelShape::CENTROID ? 0.5 : 0.0;
	double x;
	double y;
	gt.ToWorld(col + offset, row + offset, x, y);
	return OGRGeometryUniquePtr(new OGRPoint(x, y));
}

template <PixelShape SHAPE>
void PixelAsGeometryExecute(Call &c) {
	const auto geom = PixelGeometry(GeoTransform(c.Raster()), SHAPE, static_cast<int64_t>(c.Int("x")) - 1,
	                                static_cast<int64_t>(c.Int("y")) - 1);
	c.ReturnGeometry(*geom);
}

LogicalType PixelSetType() {
	child_list_t<LogicalType> children;
	children.emplace_back("geom", LogicalType::GEOMETRY());
	children.emplace_back("val", LogicalType::DOUBLE);
	children.emplace_back("x", LogicalType::INTEGER);
	children.emplace_back("y", LogicalType::INTEGER);
	return LogicalType::STRUCT(std::move(children));
}

template <PixelShape SHAPE>
void PixelAsGeometriesExecute(Call &c) {
	auto &ds = c.Raster();
	const GeoTransform gt(ds);
	const BandValues band(c.Band(), c.Bool("exclude_nodata_value", true));

	vector<Value> pixels;
	for (int y = 0; y < band.height; y++) {
		for (int x = 0; x < band.width; x++) {
			const auto idx = static_cast<idx_t>(y) * band.width + x;
			if (!band.IsValid(idx)) {
				continue;
			}
			child_list_t<Value> children;
			children.emplace_back("geom", GeometryValue(*PixelGeometry(gt, SHAPE, x, y)));
			children.emplace_back("val", Value::DOUBLE(band.values[idx]));
			children.emplace_back("x", Value::INTEGER(x + 1));
			children.emplace_back("y", Value::INTEGER(y + 1));
			pixels.push_back(Value::STRUCT(std::move(children)));
		}
	}
	c.ReturnValue(Value::LIST(PixelSetType(), std::move(pixels)));
}

void DumpValuesExecute(Call &c) {
	const BandValues band(c.Band("rast", "nband"), c.Bool("exclude_nodata_value", true));
	const auto width = static_cast<idx_t>(band.width);
	const auto height = static_cast<idx_t>(band.height);
	const auto row = c.Row();

	auto &result = c.Result();
	const auto row_offset = ListVector::GetListSize(result);
	ListVector::Reserve(result, row_offset + height);
	auto &rows = ListVector::GetEntry(result);

	const auto value_offset = ListVector::GetListSize(rows);
	ListVector::Reserve(rows, value_offset + width * height);
	auto &values = ListVector::GetEntry(rows);

	const auto row_entries = FlatVector::GetData<list_entry_t>(rows);
	const auto value_data = FlatVector::GetData<double>(values);
	auto &validity = FlatVector::Validity(values);
	for (idx_t y = 0; y < height; y++) {
		row_entries[row_offset + y] = list_entry_t(value_offset + y * width, width);
		for (idx_t x = 0; x < width; x++) {
			const auto idx = y * width + x;
			value_data[value_offset + idx] = band.values[idx];
			if (!band.IsValid(idx)) {
				validity.SetInvalid(value_offset + idx);
			}
		}
	}
	ListVector::SetListSize(rows, value_offset + width * height);
	ListVector::SetListSize(result, row_offset + height);
	FlatVector::GetData<list_entry_t>(result)[row] = list_entry_t(row_offset, height);
}

//======================================================================================================================
// Editors
//======================================================================================================================

void SetBandNoDataValueExecute(Call &c) {
	const auto ds = c.RasterCopy();
	auto &band = GetBand(*ds, c.Int("band", 1));
	SetNoData(band, c.Has("nodatavalue"), c.Double("nodatavalue", 0));
	c.ReturnRaster(*ds);
}

void SetGeoReferenceExecute(Call &c) {
	const auto ds = c.RasterCopy();
	GeoTransform gt;
	if (c.Declares("georef")) {
		const auto text = c.String("georef");
		double terms[6];
		int consumed = 0;
		if (sscanf(text.c_str(), "%lf %lf %lf %lf %lf %lf %n", &terms[0], &terms[1], &terms[2], &terms[3], &terms[4],
		           &terms[5], &consumed) != 6 ||
		    static_cast<size_t>(consumed) != text.size()) {
			throw InvalidInputException("ST_SetGeoReference: expected six numbers 'scalex skewy skewx scaley "
			                            "upperleftx upperlefty', got '%s'",
			                            text);
		}
		gt.c[1] = terms[0];
		gt.c[4] = terms[1];
		gt.c[2] = terms[2];
		gt.c[5] = terms[3];
		gt.c[0] = terms[4];
		gt.c[3] = terms[5];
		if (IsESRIFormat(c.String("format", "GDAL"))) {
			gt.c[0] -= gt.c[1] * 0.5;
			gt.c[3] -= gt.c[5] * 0.5;
		}
	} else {
		gt.c[0] = c.Double("upperleftx");
		gt.c[3] = c.Double("upperlefty");
		gt.c[1] = c.Double("scalex");
		gt.c[5] = c.Double("scaley");
		gt.c[2] = c.Double("skewx");
		gt.c[4] = c.Double("skewy");
	}
	gt.Apply(*ds);
	c.ReturnRaster(*ds);
}

void SetScaleExecute(Call &c) {
	const auto ds = c.RasterCopy();
	GeoTransform gt(*ds);
	gt.c[1] = c.Declares("scale") ? c.Double("scale") : c.Double("scalex");
	gt.c[5] = c.Declares("scale") ? c.Double("scale") : c.Double("scaley");
	gt.Apply(*ds);
	c.ReturnRaster(*ds);
}

void SetSkewExecute(Call &c) {
	const auto ds = c.RasterCopy();
	GeoTransform gt(*ds);
	gt.c[2] = c.Declares("skew") ? c.Double("skew") : c.Double("skewx");
	gt.c[4] = c.Declares("skew") ? c.Double("skew") : c.Double("skewy");
	gt.Apply(*ds);
	c.ReturnRaster(*ds);
}

void SetUpperLeftExecute(Call &c) {
	const auto ds = c.RasterCopy();
	GeoTransform gt(*ds);
	gt.c[0] = c.Double("upperleftx");
	gt.c[3] = c.Double("upperlefty");
	gt.Apply(*ds);
	c.ReturnRaster(*ds);
}

void SetSRIDExecute(Call &c) {
	const auto ds = c.RasterCopy();
	SetSRID(*ds, c.Int("srid"));
	c.ReturnRaster(*ds);
}

void SetCRSExecute(Call &c) {
	const auto ds = c.RasterCopy();
	SetCRS(*ds, c.String("crs"));
	c.ReturnRaster(*ds);
}

//======================================================================================================================
// Alignment
//======================================================================================================================

void SameAlignmentExecute(Call &c) {
	string reason;
	c.Return<bool>(SameAlignment(c.Raster("rast1"), c.Raster("rast2"), reason));
}

void NotSameAlignmentReasonExecute(Call &c) {
	string reason;
	SameAlignment(c.Raster("rast1"), c.Raster("rast2"), reason);
	c.ReturnString(reason);
}

} // namespace

//======================================================================================================================
// Registration
//======================================================================================================================

void RegisterRasterBasicFunctions(ExtensionLoader &loader) {
	const auto RASTER = RasterType();
	const auto INT = LogicalType::INTEGER;
	const auto DBL = LogicalType::DOUBLE;
	const auto BOOL = LogicalType::BOOLEAN;
	const auto TEXT = LogicalType::VARCHAR;
	const auto GEOM = LogicalType::GEOMETRY();

	RasterFunction("ST_MakeEmptyRaster")
	    .AddOptional({IntP("width"), IntP("height"), DblP("upperleftx"), DblP("upperlefty"), DblP("scalex"),
	                  DblP("scaley"), DblP("skewx"), DblP("skewy")},
	                 {IntP("srid")}, RASTER, MakeEmptyRasterExecute)
	    .Add({IntP("width"), IntP("height"), DblP("upperleftx"), DblP("upperlefty"), DblP("pixelsize")}, RASTER,
	         MakeEmptyRasterExecute)
	    .Add({RastP()}, RASTER, MakeEmptyRasterExecute)
	    .Describe(R"(
		Creates a raster without bands.

		`width` and `height` are in pixels, `upperleftx`/`upperlefty` are the world coordinates of the upper-left corner of the upper-left pixel, `scalex`/`scaley` the pixel size in world units (`scaley` is negative for north-up rasters) and `skewx`/`skewy` the rotation terms. `srid` is an EPSG code, 0 (the default) means no coordinate system. The `pixelsize` variant creates square, north-up pixels (`scalex = pixelsize`, `scaley = -pixelsize`). The single-argument variant copies the size, georeference and coordinate system of another raster.

		A raster is a `RASTER` value: a GeoTIFF byte stream. Because GeoTIFF needs at least one band, a raster without bands is stored with a placeholder band that no function exposes.
	)",
	              R"(
		SELECT ST_MetaData(ST_MakeEmptyRaster(10, 5, 100, 200, 2, -2, 0, 0, 4326));
		----
		{'upperleftx': 100.0, 'upperlefty': 200.0, 'width': 10, 'height': 5, 'scalex': 2.0, 'scaley': -2.0, 'skewx': 0.0, 'skewy': 0.0, 'srid': 4326, 'numbands': 0}
	)")
	    .Register(loader);

	RasterFunction("ST_AddBand")
	    .AddOptional({RastP(), TextP("pixeltype")}, {DblP("initialvalue"), DblP("nodataval", true)}, RASTER,
	                 AddBandExecute)
	    .AddOptional({RastP(), IntP("index"), TextP("pixeltype")}, {DblP("initialvalue"), DblP("nodataval", true)},
	                 RASTER, AddBandExecute)
	    .AddOptional({RastP("torast"), RastP("fromrast")}, {IntP("fromband"), IntP("torastindex", true)}, RASTER,
	                 AddBandFromRasterExecute)
	    .AddOptional({RastP("torast"), Param("fromrasts", LogicalType::LIST(RASTER))},
	                 {IntP("fromband"), IntP("torastindex", true)}, RASTER, AddBandFromRasterExecute)
	    .Describe(R"(
		Adds a band to a raster and returns the new raster.

		`pixeltype` is one of `8BUI`, `8BSI`, `16BUI`, `16BSI`, `32BUI`, `32BSI`, `32BF`, `64BF`. The band is filled with `initialvalue` (default 0) and gets `nodataval` as its NODATA value (default: none). `index` is the 1-based position of the new band; by default it is appended.

		The `fromrast` variants copy band `fromband` (default 1) of another raster of the same width and height, or of each raster in a list, and insert the copies at `torastindex` (default: at the end).

		Differences from PostGIS: all bands of a raster share one pixel type, so adding a band of another type is an error; `1BB`, `2BUI` and `4BUI` are accepted but stored as `8BUI`; out-of-database bands are not supported.
	)",
	              R"(
		SELECT ST_BandPixelType(ST_AddBand(ST_MakeEmptyRaster(4, 4, 0, 0, 1), '32BF', 1.5, -9999));
		----
		32BF
	)")
	    .Register(loader);

	RasterFunction("ST_Band")
	    .AddOptional({RastP()}, {IntP("nband")}, RASTER, BandExecute)
	    .Add({RastP(), Param("nbands", LogicalType::LIST(INT))}, RASTER, BandExecute)
	    .Describe(R"(
		Returns a raster made of one or more bands of the input raster.

		`nband` is a 1-based band number (default 1); `nbands` is a list of band numbers, which may repeat or reorder bands.
	)",
	              R"(
		SELECT ST_NumBands(ST_Band(rast, [3, 1])) FROM (SELECT ST_AddBand(ST_AddBand(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '8BUI'), '8BUI'), '8BUI') AS rast);
		----
		2
	)")
	    .Register(loader);

	RasterFunction("ST_NumBands")
	    .Add({RastP()}, INT, NumBandsExecute)
	    .Describe("Returns the number of bands of the raster.",
	              R"(
		SELECT ST_NumBands(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '8BUI'));
		----
		1
	)")
	    .Register(loader);

	RasterFunction("ST_Width")
	    .Add({RastP()}, INT, WidthExecute)
	    .Describe("Returns the width of the raster in pixels.",
	              R"(
		SELECT ST_Width(ST_MakeEmptyRaster(10, 5, 0, 0, 1));
		----
		10
	)")
	    .Register(loader);

	RasterFunction("ST_Height")
	    .Add({RastP()}, INT, HeightExecute)
	    .Describe("Returns the height of the raster in pixels.",
	              R"(
		SELECT ST_Height(ST_MakeEmptyRaster(10, 5, 0, 0, 1));
		----
		5
	)")
	    .Register(loader);

	RasterFunction("ST_UpperLeftX")
	    .Add({RastP()}, DBL, GeoTransformTermExecute<0>)
	    .Describe("Returns the world X coordinate of the upper-left corner of the raster.",
	              R"(
		SELECT ST_UpperLeftX(ST_MakeEmptyRaster(10, 5, 100, 200, 1));
		----
		100.0
	)")
	    .Register(loader);

	RasterFunction("ST_UpperLeftY")
	    .Add({RastP()}, DBL, GeoTransformTermExecute<3>)
	    .Describe("Returns the world Y coordinate of the upper-left corner of the raster.",
	              R"(
		SELECT ST_UpperLeftY(ST_MakeEmptyRaster(10, 5, 100, 200, 1));
		----
		200.0
	)")
	    .Register(loader);

	RasterFunction("ST_ScaleX")
	    .Add({RastP()}, DBL, GeoTransformTermExecute<1>)
	    .Describe("Returns the X term of the pixel size, in world units per pixel column.",
	              R"(
		SELECT ST_ScaleX(ST_MakeEmptyRaster(10, 5, 0, 0, 2, -3, 0, 0));
		----
		2.0
	)")
	    .Register(loader);

	RasterFunction("ST_ScaleY")
	    .Add({RastP()}, DBL, GeoTransformTermExecute<5>)
	    .Describe("Returns the Y term of the pixel size, in world units per pixel row (negative for north-up rasters).",
	              R"(
		SELECT ST_ScaleY(ST_MakeEmptyRaster(10, 5, 0, 0, 2, -3, 0, 0));
		----
		-3.0
	)")
	    .Register(loader);

	RasterFunction("ST_SkewX")
	    .Add({RastP()}, DBL, GeoTransformTermExecute<2>)
	    .Describe("Returns the X skew of the georeference: the world X offset per pixel row.",
	              R"(
		SELECT ST_SkewX(ST_MakeEmptyRaster(10, 5, 0, 0, 2, -3, 0.5, 0.25));
		----
		0.5
	)")
	    .Register(loader);

	RasterFunction("ST_SkewY")
	    .Add({RastP()}, DBL, GeoTransformTermExecute<4>)
	    .Describe("Returns the Y skew of the georeference: the world Y offset per pixel column.",
	              R"(
		SELECT ST_SkewY(ST_MakeEmptyRaster(10, 5, 0, 0, 2, -3, 0.5, 0.25));
		----
		0.25
	)")
	    .Register(loader);

	RasterFunction("ST_PixelWidth")
	    .Add({RastP()}, DBL, PixelWidthExecute)
	    .Describe("Returns the width of a pixel in world units, taking the skew into account: "
	              "`sqrt(scalex^2 + skewy^2)`.",
	              R"(
		SELECT ST_PixelWidth(ST_MakeEmptyRaster(10, 5, 0, 0, 3, -2, 0, 4));
		----
		5.0
	)")
	    .Register(loader);

	RasterFunction("ST_PixelHeight")
	    .Add({RastP()}, DBL, PixelHeightExecute)
	    .Describe("Returns the height of a pixel in world units, taking the skew into account: "
	              "`sqrt(scaley^2 + skewx^2)`.",
	              R"(
		SELECT ST_PixelHeight(ST_MakeEmptyRaster(10, 5, 0, 0, 2, -3, 4, 0));
		----
		5.0
	)")
	    .Register(loader);

	RasterFunction("ST_Rotation")
	    .Add({RastP()}, DBL, RotationExecute)
	    .Describe("Returns the rotation of the raster in radians, computed from the pixel column direction "
	              "(`scalex`, `skewy`). A raster that is not rotated returns 0.",
	              R"(
		SELECT ST_Rotation(ST_MakeEmptyRaster(10, 5, 0, 0, 2, -2, 0, 0));
		----
		0.0
	)")
	    .Register(loader);

	RasterFunction("ST_GeoReference")
	    .AddOptional({RastP()}, {TextP("format")}, TEXT, GeoReferenceExecute)
	    .Describe(R"(
		Returns the georeference as the six lines of a world file: `scalex`, `skewy`, `skewx`, `scaley`, `upperleftx`, `upperlefty`, each with 10 decimals.

		`format` is `GDAL` (the default, upper-left corner of the upper-left pixel) or `ESRI` (centre of the upper-left pixel).
	)",
	              R"(
		SELECT ST_GeoReference(ST_MakeEmptyRaster(10, 5, 100, 200, 2, -2, 0, 0));
		----
		2.0000000000
		0.0000000000
		0.0000000000
		-2.0000000000
		100.0000000000
		200.0000000000
	)")
	    .Register(loader);

	RasterFunction("ST_MetaData")
	    .Add({RastP()}, MetaDataType(), MetaDataExecute)
	    .Describe("Returns the size, georeference, SRID (0 when the coordinate system is not an EPSG code) and number "
	              "of bands of the raster as a struct.",
	              R"(
		SELECT ST_MetaData(ST_MakeEmptyRaster(10, 5, 100, 200, 2, -2, 0, 0, 4326));
		----
		{'upperleftx': 100.0, 'upperlefty': 200.0, 'width': 10, 'height': 5, 'scalex': 2.0, 'scaley': -2.0, 'skewx': 0.0, 'skewy': 0.0, 'srid': 4326, 'numbands': 0}
	)")
	    .Register(loader);

	RasterFunction("ST_BandMetaData")
	    .AddOptional({RastP()}, {IntP("band")}, BandMetaDataType(), BandMetaDataExecute)
	    .Describe("Returns the pixel type and NODATA value (NULL if none) of a band (1-based, default 1) as a struct. "
	              "`isoutdb` is always false and `path` always NULL: bands are always stored in the raster value.",
	              R"(
		SELECT ST_BandMetaData(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '16BSI', 0, -1));
		----
		{'pixeltype': 16BSI, 'nodatavalue': -1.0, 'isoutdb': false, 'path': NULL}
	)")
	    .Register(loader);

	RasterFunction("ST_BandPixelType")
	    .AddOptional({RastP()}, {IntP("band")}, TEXT, BandPixelTypeExecute)
	    .Describe("Returns the pixel type of a band (1-based, default 1): `8BUI`, `8BSI`, `16BUI`, `16BSI`, `32BUI`, "
	              "`32BSI`, `32BF` or `64BF`.",
	              R"(
		SELECT ST_BandPixelType(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '16BSI'));
		----
		16BSI
	)")
	    .Register(loader);

	RasterFunction("ST_BandNoDataValue")
	    .AddOptional({RastP()}, {IntP("band")}, DBL, BandNoDataValueExecute)
	    .Describe("Returns the NODATA value of a band (1-based, default 1), or NULL if the band has none.",
	              R"(
		SELECT ST_BandNoDataValue(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '16BSI', 0, -1));
		----
		-1.0
	)")
	    .Register(loader);

	RasterFunction("ST_HasNoBand")
	    .AddOptional({RastP()}, {IntP("band")}, BOOL, HasNoBandExecute)
	    .Describe("Returns true if the raster has no band with the given number (1-based, default 1).",
	              R"(
		SELECT ST_HasNoBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1));
		----
		true
	)")
	    .Register(loader);

	RasterFunction("ST_BandIsNoData")
	    .AddOptional({RastP()}, {IntP("band"), BoolP("forcechecking")}, BOOL, BandIsNoDataExecute)
	    .Add({RastP(), BoolP("forcechecking")}, BOOL, BandIsNoDataExecute)
	    .Describe("Returns true if every pixel of the band (1-based, default 1) is NODATA. The pixels are always "
	              "inspected, so `forcechecking` is accepted for compatibility and ignored.",
	              R"(
		SELECT ST_BandIsNoData(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '8BUI', 7, 7));
		----
		true
	)")
	    .Register(loader);

	RasterFunction("ST_SRID")
	    .Add({RastP()}, INT, SRIDExecute)
	    .Describe("Returns the EPSG code of the raster's coordinate system, or 0 if it has none or is not an EPSG "
	              "coordinate system (see ST_CRS).",
	              R"(
		SELECT ST_SRID(ST_MakeEmptyRaster(2, 2, 0, 0, 1, -1, 0, 0, 3857));
		----
		3857
	)")
	    .Register(loader);

	RasterFunction("ST_CRS")
	    .Add({RastP()}, TEXT, CRSExecute)
	    .Describe("Returns the coordinate system of the raster as `AUTHORITY:CODE` when it has an authority code, as "
	              "WKT otherwise, or NULL if the raster has none.",
	              R"(
		SELECT ST_CRS(ST_MakeEmptyRaster(2, 2, 0, 0, 1, -1, 0, 0, 3857));
		----
		EPSG:3857
	)")
	    .Register(loader);

	RasterFunction("ST_Envelope")
	    .Add({RastP()}, GEOM, EnvelopeExecute)
	    .Describe("Returns the axis-aligned bounding box of the raster as a polygon in world coordinates.",
	              R"(
		SELECT ST_Envelope(ST_MakeEmptyRaster(10, 5, 100, 200, 2, -2, 0, 0));
		----
		POLYGON ((100 190, 120 190, 120 200, 100 200, 100 190))
	)")
	    .Register(loader);

	RasterFunction("ST_ConvexHull")
	    .Add({RastP()}, GEOM, ConvexHullExecute)
	    .Describe("Returns the outline of the raster as a polygon through its four corners, starting at the upper-left "
	              "one. For a skewed raster this is a parallelogram, unlike ST_Envelope.",
	              R"(
		SELECT ST_ConvexHull(ST_MakeEmptyRaster(10, 5, 100, 200, 2, -2, 0, 0));
		----
		POLYGON ((100 200, 120 200, 120 190, 100 190, 100 200))
	)")
	    .Register(loader);

	RasterFunction("ST_MinConvexHull")
	    .AddOptional({RastP()}, {IntP("nband", true)}, GEOM, MinConvexHullExecute)
	    .Describe("Returns the outline of the smallest pixel window that contains every pixel that is not NODATA, in "
	              "band `nband` (1-based) or in any band when omitted or NULL. Returns NULL if all pixels are NODATA.",
	              R"(
		SELECT ST_MinConvexHull(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(4, 4, 0, 0, 1), '8BUI', 0, 0), 2, 2, 9));
		----
		POLYGON ((1 -1, 2 -1, 2 -2, 1 -2, 1 -1))
	)")
	    .Register(loader);

	RasterFunction("ST_RasterToWorldCoord")
	    .Add({RastP(), IntP("columnx"), IntP("rowy")}, WorldCoordType(), RasterToWorldCoordExecute)
	    .Describe("Returns the world coordinates of the upper-left corner of a pixel as a struct. Pixel columns and "
	              "rows are numbered from 1 and may lie outside of the raster.",
	              R"(
		SELECT ST_RasterToWorldCoord(ST_MakeEmptyRaster(10, 5, 100, 200, 2, -2, 0, 0), 2, 3);
		----
		{'longitude': 102.0, 'latitude': 196.0}
	)")
	    .Register(loader);

	RasterFunction("ST_RasterToWorldCoordX")
	    .Add({RastP(), IntP("xr"), IntP("yr")}, DBL, RasterToWorldCoordAxisExecute<true>)
	    .Add({RastP(), IntP("xr")}, DBL, RasterToWorldCoordAxisExecute<true>)
	    .Describe("Returns the world X coordinate of the upper-left corner of a pixel (columns and rows numbered from "
	              "1). The row may be omitted if the raster is not skewed.",
	              R"(
		SELECT ST_RasterToWorldCoordX(ST_MakeEmptyRaster(10, 5, 100, 200, 2, -2, 0, 0), 2);
		----
		102.0
	)")
	    .Register(loader);

	RasterFunction("ST_RasterToWorldCoordY")
	    .Add({RastP(), IntP("xr"), IntP("yr")}, DBL, RasterToWorldCoordAxisExecute<false>)
	    .Add({RastP(), IntP("yr")}, DBL, RasterToWorldCoordAxisExecute<false>)
	    .Describe("Returns the world Y coordinate of the upper-left corner of a pixel (columns and rows numbered from "
	              "1). The column may be omitted if the raster is not skewed.",
	              R"(
		SELECT ST_RasterToWorldCoordY(ST_MakeEmptyRaster(10, 5, 100, 200, 2, -2, 0, 0), 3);
		----
		196.0
	)")
	    .Register(loader);

	RasterFunction("ST_WorldToRasterCoord")
	    .Add({RastP(), DblP("xw"), DblP("yw")}, RasterCoordType(), WorldToRasterCoordExecute)
	    .Add({RastP(), GeomP("pt")}, RasterCoordType(), WorldToRasterCoordExecute)
	    .Describe("Returns the 1-based column and row of the pixel that contains a world coordinate or a point, as a "
	              "struct. The result may lie outside of the raster.",
	              R"(
		SELECT ST_WorldToRasterCoord(ST_MakeEmptyRaster(10, 5, 100, 200, 2, -2, 0, 0), 103, 195);
		----
		{'columnx': 2, 'rowy': 3}
	)")
	    .Register(loader);

	RasterFunction("ST_WorldToRasterCoordX")
	    .Add({RastP(), DblP("xw"), DblP("yw")}, INT, WorldToRasterCoordAxisExecute<true>)
	    .Add({RastP(), DblP("xw")}, INT, WorldToRasterCoordAxisExecute<true>)
	    .Add({RastP(), GeomP("pt")}, INT, WorldToRasterCoordAxisExecute<true>)
	    .Describe("Returns the 1-based column of the pixel that contains a world coordinate or a point. `yw` may be "
	              "omitted if the raster is not skewed.",
	              R"(
		SELECT ST_WorldToRasterCoordX(ST_MakeEmptyRaster(10, 5, 100, 200, 2, -2, 0, 0), 103);
		----
		2
	)")
	    .Register(loader);

	RasterFunction("ST_WorldToRasterCoordY")
	    .Add({RastP(), DblP("xw"), DblP("yw")}, INT, WorldToRasterCoordAxisExecute<false>)
	    .Add({RastP(), DblP("yw")}, INT, WorldToRasterCoordAxisExecute<false>)
	    .Add({RastP(), GeomP("pt")}, INT, WorldToRasterCoordAxisExecute<false>)
	    .Describe("Returns the 1-based row of the pixel that contains a world coordinate or a point. `xw` may be "
	              "omitted if the raster is not skewed.",
	              R"(
		SELECT ST_WorldToRasterCoordY(ST_MakeEmptyRaster(10, 5, 100, 200, 2, -2, 0, 0), 195);
		----
		3
	)")
	    .Register(loader);

	RasterFunction("ST_Value")
	    .AddOptional({RastP(), IntP("x"), IntP("y")}, {BoolP("exclude_nodata_value")}, DBL, ValueExecute)
	    .AddOptional({RastP(), IntP("band"), IntP("x"), IntP("y")}, {BoolP("exclude_nodata_value")}, DBL, ValueExecute)
	    .AddOptional({RastP(), GeomP("pt")}, {BoolP("exclude_nodata_value")}, DBL, ValueExecute)
	    .AddOptional({RastP(), IntP("band"), GeomP("pt")}, {BoolP("exclude_nodata_value")}, DBL, ValueExecute)
	    .Describe(R"(
		Returns the value of a pixel, addressed by its 1-based column `x` and row `y` or by a point in world coordinates.

		`band` is 1-based and defaults to 1. NODATA pixels return NULL unless `exclude_nodata_value` is false. A pixel or point outside of the raster returns NULL. The point is not reprojected and only nearest-pixel lookup is supported (PostGIS's `resample` argument is not available).
	)",
	              R"(
		SELECT ST_Value(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(4, 4, 0, 0, 1), '8BUI'), 2, 3, 42), 2, 3);
		----
		42.0
	)")
	    .Register(loader);

	RasterFunction("ST_SetValue")
	    .Add({RastP(), IntP("x"), IntP("y"), DblP("newvalue", true)}, RASTER, SetValueExecute)
	    .Add({RastP(), IntP("band"), IntP("x"), IntP("y"), DblP("newvalue", true)}, RASTER, SetValueExecute)
	    .Add({RastP(), GeomP("geom"), DblP("newvalue", true)}, RASTER, SetValueExecute)
	    .Add({RastP(), IntP("band"), GeomP("geom"), DblP("newvalue", true)}, RASTER, SetValueExecute)
	    .Describe(R"(
		Returns the raster with one pixel (1-based column `x` and row `y`), or every pixel covered by a geometry, set to `newvalue` in a band (1-based, default 1).

		A NULL `newvalue` sets the pixels to the band's NODATA value. The value is clamped to the range of the pixel type. For polygons the pixels whose centre is inside are set, for lines and points the pixels they pass through. A pixel outside of the raster is an error.
	)",
	              R"(
		SELECT ST_DumpValues(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '8BUI'), 1, 2, 5));
		----
		[[0.0, 0.0], [5.0, 0.0]]
	)")
	    .Register(loader);

	RasterFunction("ST_NearestValue")
	    .AddOptional({RastP(), GeomP("pt")}, {BoolP("exclude_nodata_value")}, DBL, NearestValueExecute)
	    .AddOptional({RastP(), IntP("band"), GeomP("pt")}, {BoolP("exclude_nodata_value")}, DBL, NearestValueExecute)
	    .AddOptional({RastP(), IntP("columnx"), IntP("rowy")}, {BoolP("exclude_nodata_value")}, DBL,
	                 NearestValueExecute)
	    .AddOptional({RastP(), IntP("band"), IntP("columnx"), IntP("rowy")}, {BoolP("exclude_nodata_value")}, DBL,
	                 NearestValueExecute)
	    .Describe(R"(
		Returns the value of the pixel at a point or at a 1-based column and row if it is not NODATA, otherwise the value of the nearest pixel that is not NODATA.

		The distance is measured in world units from the point (or the centre of the given pixel) to the pixel centres. The location may be outside of the raster. Returns NULL if the band (1-based, default 1) has no such pixel.
	)",
	              R"(
		SELECT ST_NearestValue(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(4, 4, 0, 0, 1), '8BUI', 0, 0), 4, 4, 9), 1, 1);
		----
		9.0
	)")
	    .Register(loader);

	RasterFunction("ST_PixelAsPoint")
	    .Add({RastP(), IntP("x"), IntP("y")}, GEOM, PixelAsGeometryExecute<PixelShape::POINT>)
	    .Describe("Returns the upper-left corner of a pixel (1-based column `x` and row `y`) as a point.",
	              R"(
		SELECT ST_PixelAsPoint(ST_MakeEmptyRaster(10, 5, 100, 200, 2, -2, 0, 0), 2, 3);
		----
		POINT (102 196)
	)")
	    .Register(loader);

	RasterFunction("ST_PixelAsCentroid")
	    .Add({RastP(), IntP("x"), IntP("y")}, GEOM, PixelAsGeometryExecute<PixelShape::CENTROID>)
	    .Describe("Returns the centre of a pixel (1-based column `x` and row `y`) as a point.",
	              R"(
		SELECT ST_PixelAsCentroid(ST_MakeEmptyRaster(10, 5, 100, 200, 2, -2, 0, 0), 2, 3);
		----
		POINT (103 195)
	)")
	    .Register(loader);

	RasterFunction("ST_PixelAsPolygon")
	    .Add({RastP(), IntP("x"), IntP("y")}, GEOM, PixelAsGeometryExecute<PixelShape::POLYGON>)
	    .Describe("Returns the outline of a pixel (1-based column `x` and row `y`) as a polygon.",
	              R"(
		SELECT ST_PixelAsPolygon(ST_MakeEmptyRaster(10, 5, 100, 200, 2, -2, 0, 0), 2, 3);
		----
		POLYGON ((102 196, 104 196, 104 194, 102 194, 102 196))
	)")
	    .Register(loader);

	const auto pixel_list = LogicalType::LIST(PixelSetType());
	const char *pixel_set_description = R"(
		Returns one entry per pixel of a band as a list of structs `(geom, val, x, y)`: the pixel's %s, its value and its 1-based column and row.

		`band` is 1-based and defaults to 1. NODATA pixels are left out unless `exclude_nodata_value` is false. PostGIS returns a set of rows; use `UNNEST(..., recursive := true)` to get the same shape.
	)";
	const auto points_description = StringUtil::Format(pixel_set_description, "upper-left corner as a point");
	const auto centroids_description = StringUtil::Format(pixel_set_description, "centre as a point");
	const auto polygons_description = StringUtil::Format(pixel_set_description, "outline as a polygon");

	RasterFunction("ST_PixelAsPoints")
	    .AddOptional({RastP()}, {IntP("band"), BoolP("exclude_nodata_value")}, pixel_list,
	                 PixelAsGeometriesExecute<PixelShape::POINT>)
	    .Describe(points_description.c_str(),
	              R"(
		SELECT UNNEST(ST_PixelAsPoints(ST_AddBand(ST_MakeEmptyRaster(2, 1, 0, 0, 1), '8BUI', 7)), recursive := true);
		----
		POINT (0 0)	7.0	1	1
		POINT (1 0)	7.0	2	1
	)")
	    .Register(loader);

	RasterFunction("ST_PixelAsCentroids")
	    .AddOptional({RastP()}, {IntP("band"), BoolP("exclude_nodata_value")}, pixel_list,
	                 PixelAsGeometriesExecute<PixelShape::CENTROID>)
	    .Describe(centroids_description.c_str(),
	              R"(
		SELECT UNNEST(ST_PixelAsCentroids(ST_AddBand(ST_MakeEmptyRaster(2, 1, 0, 0, 1), '8BUI', 7)), recursive := true);
		----
		POINT (0.5 -0.5)	7.0	1	1
		POINT (1.5 -0.5)	7.0	2	1
	)")
	    .Register(loader);

	RasterFunction("ST_PixelAsPolygons")
	    .AddOptional({RastP()}, {IntP("band"), BoolP("exclude_nodata_value")}, pixel_list,
	                 PixelAsGeometriesExecute<PixelShape::POLYGON>)
	    .Describe(polygons_description.c_str(),
	              R"(
		SELECT UNNEST(ST_PixelAsPolygons(ST_AddBand(ST_MakeEmptyRaster(2, 1, 0, 0, 1), '8BUI', 7)), recursive := true);
		----
		POLYGON ((0 0, 1 0, 1 -1, 0 -1, 0 0))	7.0	1	1
		POLYGON ((1 0, 2 0, 2 -1, 1 -1, 1 0))	7.0	2	1
	)")
	    .Register(loader);

	RasterFunction("ST_DumpValues")
	    .AddOptional({RastP()}, {IntP("nband"), BoolP("exclude_nodata_value")}, LogicalType::LIST(LogicalType::LIST(DBL)),
	                 DumpValuesExecute)
	    .Describe(R"(
		Returns the pixel values of a band as a list of rows, each a list of values: `result[y][x]` is the pixel at column `x`, row `y` (both 1-based).

		`nband` is 1-based and defaults to 1. NODATA pixels are NULL unless `exclude_nodata_value` is false. The variant of PostGIS that returns one row per band is not available: call the function once per band.
	)",
	              R"(
		SELECT ST_DumpValues(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '8BUI', 1, 0), 2, 1, NULL));
		----
		[[1.0, NULL], [1.0, 1.0]]
	)")
	    .Register(loader);

	RasterFunction("ST_SetBandNoDataValue")
	    .Add({RastP(), DblP("nodatavalue", true)}, RASTER, SetBandNoDataValueExecute)
	    .AddOptional({RastP(), IntP("band"), DblP("nodatavalue", true)}, {BoolP("forcechecking")}, RASTER,
	                 SetBandNoDataValueExecute)
	    .Describe("Returns the raster with the NODATA value of a band (1-based, default 1) set to `nodatavalue`. NULL "
	              "removes the NODATA value. The pixel values do not change. `forcechecking` is accepted for "
	              "compatibility and ignored.",
	              R"(
		SELECT ST_BandNoDataValue(ST_SetBandNoDataValue(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '8BUI'), 255));
		----
		255.0
	)")
	    .Register(loader);

	RasterFunction("ST_SetGeoReference")
	    .AddOptional({RastP(), TextP("georef")}, {TextP("format")}, RASTER, SetGeoReferenceExecute)
	    .Add({RastP(), DblP("upperleftx"), DblP("upperlefty"), DblP("scalex"), DblP("scaley"), DblP("skewx"),
	          DblP("skewy")},
	         RASTER, SetGeoReferenceExecute)
	    .Describe(R"(
		Returns the raster with a new georeference. The pixels are not resampled.

		`georef` holds the six world-file terms `scalex skewy skewx scaley upperleftx upperlefty` separated by whitespace. `format` is `GDAL` (the default, the upper-left term is the corner of the upper-left pixel) or `ESRI` (it is the centre of that pixel).
	)",
	              R"(
		SELECT ST_UpperLeftX(ST_SetGeoReference(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '2 0 0 -2 100 200'));
		----
		100.0
	)")
	    .Register(loader);

	RasterFunction("ST_SetScale")
	    .Add({RastP(), DblP("scale")}, RASTER, SetScaleExecute)
	    .Add({RastP(), DblP("scalex"), DblP("scaley")}, RASTER, SetScaleExecute)
	    .Describe("Returns the raster with a new pixel size, in world units. The pixels are not resampled (see "
	              "ST_Rescale). The single-value variant sets both `scalex` and `scaley` to the same value.",
	              R"(
		SELECT ST_ScaleY(ST_SetScale(ST_MakeEmptyRaster(2, 2, 0, 0, 1), 2, -3));
		----
		-3.0
	)")
	    .Register(loader);

	RasterFunction("ST_SetSkew")
	    .Add({RastP(), DblP("skew")}, RASTER, SetSkewExecute)
	    .Add({RastP(), DblP("skewx"), DblP("skewy")}, RASTER, SetSkewExecute)
	    .Describe("Returns the raster with new skew terms. The pixels are not resampled (see ST_Reskew). The "
	              "single-value variant sets both `skewx` and `skewy` to the same value.",
	              R"(
		SELECT ST_SkewX(ST_SetSkew(ST_MakeEmptyRaster(2, 2, 0, 0, 1), 0.5, 0.25));
		----
		0.5
	)")
	    .Register(loader);

	RasterFunction("ST_SetUpperLeft")
	    .Add({RastP(), DblP("upperleftx"), DblP("upperlefty")}, RASTER, SetUpperLeftExecute)
	    .Describe("Returns the raster moved so that its upper-left corner is at the given world coordinates.",
	              R"(
		SELECT ST_UpperLeftY(ST_SetUpperLeft(ST_MakeEmptyRaster(2, 2, 0, 0, 1), 100, 200));
		----
		200.0
	)")
	    .Register(loader);

	RasterFunction("ST_SetSRID")
	    .Add({RastP(), IntP("srid")}, RASTER, SetSRIDExecute)
	    .Describe("Returns the raster with its coordinate system set to an EPSG code, without reprojecting the pixels "
	              "(see ST_Transform). 0 removes the coordinate system.",
	              R"(
		SELECT ST_SRID(ST_SetSRID(ST_MakeEmptyRaster(2, 2, 0, 0, 1), 31370));
		----
		31370
	)")
	    .Register(loader);

	RasterFunction("ST_SetCRS")
	    .Add({RastP(), TextP("crs")}, RASTER, SetCRSExecute)
	    .Describe("Returns the raster with its coordinate system set from an `AUTHORITY:CODE` string, WKT or a PROJ "
	              "string, without reprojecting the pixels (see ST_Transform). An empty string removes the coordinate "
	              "system.",
	              R"(
		SELECT ST_CRS(ST_SetCRS(ST_MakeEmptyRaster(2, 2, 0, 0, 1), 'EPSG:31370'));
		----
		EPSG:31370
	)")
	    .Register(loader);

	RasterFunction("ST_SameAlignment")
	    .Add({RastP("rast1"), RastP("rast2")}, BOOL, SameAlignmentExecute)
	    .Describe("Returns true if two rasters have the same coordinate system, scale and skew and their pixel grids "
	              "line up (a pixel corner of one falls on a pixel corner of the other). The rasters do not need to "
	              "overlap.",
	              R"(
		SELECT ST_SameAlignment(ST_MakeEmptyRaster(2, 2, 0, 0, 1), ST_MakeEmptyRaster(3, 3, 5, -7, 1));
		----
		true
	)")
	    .Register(loader);

	RasterFunction("ST_NotSameAlignmentReason")
	    .Add({RastP("rast1"), RastP("rast2")}, TEXT, NotSameAlignmentReasonExecute)
	    .Describe("Returns the reason why two rasters are not aligned (see ST_SameAlignment), or 'The rasters are "
	              "aligned'.",
	              R"(
		SELECT ST_NotSameAlignmentReason(ST_MakeEmptyRaster(2, 2, 0, 0, 1), ST_MakeEmptyRaster(2, 2, 0.5, 0, 1));
		----
		The rasters (pixel corner coordinates) are not aligned
	)")
	    .Register(loader);
}

} // namespace raster
} // namespace duckdb
