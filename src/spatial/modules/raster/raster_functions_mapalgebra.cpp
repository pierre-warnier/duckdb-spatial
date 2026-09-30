#include "spatial/modules/raster/raster_core.hpp"

#include "duckdb/execution/expression_executor.hpp"
#include "duckdb/parser/expression/columnref_expression.hpp"
#include "duckdb/parser/parser.hpp"
#include "duckdb/planner/binder.hpp"
#include "duckdb/planner/expression/bound_cast_expression.hpp"
#include "duckdb/planner/expression/bound_function_expression.hpp"
#include "duckdb/planner/expression/bound_reference_expression.hpp"
#include "duckdb/planner/expression_binder.hpp"

namespace duckdb {
namespace raster {

namespace {

//======================================================================================================================
// Pixel expressions
//======================================================================================================================
// A map algebra expression is a SQL expression over the PostGIS placeholders [rast], [rast.x], ... It is parsed and
// bound once, with DuckDB's own parser and binder, against a synthetic row that has one column per placeholder, and
// then evaluated over chunks of pixels.

struct Placeholder {
	const char *name;
	idx_t column;
};

// Columns of the pixel chunk: value, x, y of the first raster, then of the second one
constexpr idx_t VALUE_1 = 0;
constexpr idx_t X_1 = 1;
constexpr idx_t Y_1 = 2;
constexpr idx_t VALUE_2 = 3;
constexpr idx_t X_2 = 4;
constexpr idx_t Y_2 = 5;

const Placeholder ONE_RASTER_PLACEHOLDERS[] = {
    {"[rast]", VALUE_1}, {"[rast.val]", VALUE_1}, {"[rast.x]", X_1}, {"[rast.y]", Y_1}};

const Placeholder TWO_RASTER_PLACEHOLDERS[] = {{"[rast1]", VALUE_1},     {"[rast1.val]", VALUE_1}, {"[rast1.x]", X_1},
                                               {"[rast1.y]", Y_1},       {"[rast2]", VALUE_2},     {"[rast2.val]", VALUE_2},
                                               {"[rast2.x]", X_2},       {"[rast2.y]", Y_2}};

vector<Placeholder> Placeholders(bool two_rasters) {
	if (two_rasters) {
		return vector<Placeholder>(std::begin(TWO_RASTER_PLACEHOLDERS), std::end(TWO_RASTER_PLACEHOLDERS));
	}
	return vector<Placeholder>(std::begin(ONE_RASTER_PLACEHOLDERS), std::end(ONE_RASTER_PLACEHOLDERS));
}

vector<LogicalType> PixelChunkTypes(bool two_rasters) {
	vector<LogicalType> types = {LogicalType::DOUBLE, LogicalType::INTEGER, LogicalType::INTEGER};
	if (two_rasters) {
		types.insert(types.end(), {LogicalType::DOUBLE, LogicalType::INTEGER, LogicalType::INTEGER});
	}
	return types;
}

// Turns every placeholder into a quoted identifier, so that the expression parses as plain SQL
string QuotePlaceholders(const string &expression, const vector<Placeholder> &placeholders) {
	string result;
	auto in_string = false;
	for (idx_t pos = 0; pos < expression.size();) {
		const auto ch = expression[pos];
		if (ch == '\'') {
			in_string = !in_string;
		}
		if (ch == '[' && !in_string) {
			auto matched = false;
			for (const auto &placeholder : placeholders) {
				const auto length = strlen(placeholder.name);
				if (expression.size() - pos >= length &&
				    StringUtil::CIEquals(expression.substr(pos, length), placeholder.name)) {
					result += "\"";
					result += placeholder.name;
					result += "\"";
					pos += length;
					matched = true;
					break;
				}
			}
			if (matched) {
				continue;
			}
		}
		result += ch;
		pos++;
	}
	return result;
}

class PixelExpressionBinder final : public ExpressionBinder {
public:
	PixelExpressionBinder(Binder &binder, ClientContext &context, const vector<Placeholder> &placeholders_p,
	                      const vector<LogicalType> &types_p)
	    : ExpressionBinder(binder, context), placeholders(placeholders_p), types(types_p) {
		target_type = LogicalType::DOUBLE;
	}

protected:
	BindResult BindExpression(unique_ptr<ParsedExpression> &expr_ptr, idx_t depth, bool root_expression) override {
		auto &expr = *expr_ptr;
		switch (expr.GetExpressionClass()) {
		case ExpressionClass::WINDOW:
			return BindResult("window functions are not allowed in a map algebra expression");
		case ExpressionClass::SUBQUERY:
			return BindResult("subqueries are not allowed in a map algebra expression");
		case ExpressionClass::COLUMN_REF: {
			auto &colref = expr.Cast<ColumnRefExpression>();
			if (!colref.IsQualified()) {
				for (const auto &placeholder : placeholders) {
					if (StringUtil::CIEquals(colref.GetColumnName(), placeholder.name)) {
						return BindResult(make_uniq<BoundReferenceExpression>(placeholder.name,
						                                                     types[placeholder.column],
						                                                     placeholder.column));
					}
				}
			}
			return ExpressionBinder::BindExpression(expr_ptr, depth, root_expression);
		}
		default:
			return ExpressionBinder::BindExpression(expr_ptr, depth, root_expression);
		}
	}

	string UnsupportedAggregateMessage() override {
		return "aggregate functions are not allowed in a map algebra expression";
	}

private:
	const vector<Placeholder> &placeholders;
	const vector<LogicalType> &types;
};

unique_ptr<Expression> BindPixelExpression(ClientContext &context, const string &expression, bool two_rasters) {
	const auto placeholders = Placeholders(two_rasters);
	const auto types = PixelChunkTypes(two_rasters);
	try {
		auto parsed = Parser::ParseExpressionList(QuotePlaceholders(expression, placeholders),
		                                          context.GetParserOptions());
		if (parsed.size() != 1) {
			throw BinderException("expected a single expression");
		}
		const auto binder = Binder::CreateBinder(context);
		PixelExpressionBinder expression_binder(*binder, context, placeholders, types);
		return expression_binder.Bind(parsed[0]);
	} catch (std::exception &ex) {
		ErrorData error(ex);
		if (error.Type() == ExceptionType::PARAMETER_NOT_RESOLVED) {
			throw;
		}
		throw BinderException("ST_MapAlgebra: invalid expression '%s': %s. The expression is a SQL expression over "
		                      "%s",
		                      expression, error.RawMessage(),
		                      two_rasters ? "[rast1], [rast1.x], [rast1.y], [rast2], [rast2.x] and [rast2.y]"
		                                  : "[rast], [rast.x] and [rast.y]");
	}
}

//======================================================================================================================
// Bind data and local state
//======================================================================================================================

struct MapAlgebraBindData final : public FunctionData {
	shared_ptr<vector<Param>> params;
	bool two_rasters = false;
	unique_ptr<Expression> expression;
	unique_ptr<Expression> nodata1;
	unique_ptr<Expression> nodata2;

	unique_ptr<FunctionData> Copy() const override {
		auto result = make_uniq<MapAlgebraBindData>();
		result->params = params;
		result->two_rasters = two_rasters;
		result->expression = expression ? expression->Copy() : nullptr;
		result->nodata1 = nodata1 ? nodata1->Copy() : nullptr;
		result->nodata2 = nodata2 ? nodata2->Copy() : nullptr;
		return std::move(result);
	}

	static bool SameExpression(const unique_ptr<Expression> &a, const unique_ptr<Expression> &b) {
		return (!a && !b) || (a && b && a->Equals(*b));
	}

	bool Equals(const FunctionData &other_p) const override {
		auto &other = other_p.Cast<MapAlgebraBindData>();
		return params == other.params && SameExpression(expression, other.expression) &&
		       SameExpression(nodata1, other.nodata1) && SameExpression(nodata2, other.nodata2);
	}
};

unique_ptr<Expression> BindExpressionArgument(ClientContext &context, const vector<Param> &params,
                                              vector<unique_ptr<Expression>> &arguments, const char *name,
                                              bool two_rasters) {
	for (idx_t i = 0; i < params.size(); i++) {
		if (strcmp(params[i].name, name) != 0) {
			continue;
		}
		if (arguments[i]->HasParameter()) {
			throw ParameterNotResolvedException();
		}
		if (!arguments[i]->IsFoldable()) {
			throw BinderException("ST_MapAlgebra: the '%s' argument must be a constant", name);
		}
		const auto value = ExpressionExecutor::EvaluateScalar(context, *arguments[i]);
		if (value.IsNull()) {
			return nullptr;
		}
		return BindPixelExpression(context, value.ToString(), two_rasters);
	}
	return nullptr;
}

unique_ptr<FunctionData> MapAlgebraBind(ClientContext &context, ScalarFunction &bound_function,
                                        vector<unique_ptr<Expression>> &arguments) {
	auto result = make_uniq<MapAlgebraBindData>();
	result->params = bound_function.GetExtraFunctionInfo().Cast<ParamsInfo>().params;
	const auto &params = *result->params;
	result->two_rasters = strcmp(params[0].name, "rast1") == 0;
	result->expression = BindExpressionArgument(context, params, arguments, "expression", result->two_rasters);
	result->nodata1 = BindExpressionArgument(context, params, arguments, "nodata1expr", result->two_rasters);
	result->nodata2 = BindExpressionArgument(context, params, arguments, "nodata2expr", result->two_rasters);
	return std::move(result);
}

struct MapAlgebraLocalState final : public FunctionLocalState {
	DataChunk input;
	DataChunk output;
	unique_ptr<ExpressionExecutor> expression;
	unique_ptr<ExpressionExecutor> nodata1;
	unique_ptr<ExpressionExecutor> nodata2;
	// Results of the last evaluation
	double values[STANDARD_VECTOR_SIZE];
	bool valid[STANDARD_VECTOR_SIZE];

	void Evaluate(ExpressionExecutor &executor, idx_t count) {
		input.SetCardinality(count);
		output.Reset();
		executor.Execute(input, output);
		UnifiedVectorFormat format;
		output.data[0].ToUnifiedFormat(count, format);
		const auto data = UnifiedVectorFormat::GetData<double>(format);
		for (idx_t i = 0; i < count; i++) {
			const auto idx = format.sel->get_index(i);
			valid[i] = format.validity.RowIsValid(idx);
			values[i] = valid[i] ? data[idx] : 0;
		}
	}
};

unique_ptr<FunctionLocalState> MapAlgebraInit(ExpressionState &state, const BoundFunctionExpression &expr,
                                              FunctionData *bind_data_p) {
	auto &bind_data = bind_data_p->Cast<MapAlgebraBindData>();
	auto &context = state.GetContext();
	auto result = make_uniq<MapAlgebraLocalState>();
	result->input.Initialize(Allocator::Get(context), PixelChunkTypes(bind_data.two_rasters));
	result->output.Initialize(Allocator::Get(context), {LogicalType::DOUBLE});
	if (bind_data.expression) {
		result->expression = make_uniq<ExpressionExecutor>(context, *bind_data.expression);
	}
	if (bind_data.nodata1) {
		result->nodata1 = make_uniq<ExpressionExecutor>(context, *bind_data.nodata1);
	}
	if (bind_data.nodata2) {
		result->nodata2 = make_uniq<ExpressionExecutor>(context, *bind_data.nodata2);
	}
	return std::move(result);
}

//======================================================================================================================
// Execution
//======================================================================================================================

// Writes the computed pixels as a single-band raster, giving the pixels without a result the NODATA value
void ReturnBand(Call &c, GDALDataset &result, GDALDataType type, vector<double> &values, const vector<bool> &valid,
                bool has_nodata, double nodata) {
	auto any_invalid = false;
	for (idx_t i = 0; i < values.size() && !any_invalid; i++) {
		any_invalid = !valid[i];
	}
	if (any_invalid && !has_nodata) {
		has_nodata = true;
		nodata = MinPossibleValue(type);
	}
	if (has_nodata) {
		nodata = ClampToPixelType(nodata, type);
		for (idx_t i = 0; i < values.size(); i++) {
			if (!valid[i]) {
				values[i] = nodata;
			}
		}
	}
	AddBand(result, type);
	auto &band = *result.GetRasterBand(1);
	SetNoData(band, has_nodata, nodata);
	WriteBand(band, values);
	c.ReturnRaster(result);
}

void OneRasterExecute(Call &c, MapAlgebraLocalState &local) {
	auto &src = c.Raster();
	auto &src_band = c.Band("rast", "nband");
	const BandValues source(src_band, true);
	const auto type = c.Has("pixeltype") ? ParsePixelType(c.String("pixeltype")) : src_band.GetRasterDataType();
	const auto width = static_cast<idx_t>(source.width);
	const auto pixel_count = source.values.size();

	vector<double> values(pixel_count);
	vector<bool> valid(pixel_count);
	const auto input_values = FlatVector::GetData<double>(local.input.data[VALUE_1]);
	const auto input_x = FlatVector::GetData<int32_t>(local.input.data[X_1]);
	const auto input_y = FlatVector::GetData<int32_t>(local.input.data[Y_1]);

	for (idx_t offset = 0; offset < pixel_count; offset += STANDARD_VECTOR_SIZE) {
		const auto count = MinValue<idx_t>(STANDARD_VECTOR_SIZE, pixel_count - offset);
		local.input.Reset();
		auto &validity = FlatVector::Validity(local.input.data[VALUE_1]);
		for (idx_t i = 0; i < count; i++) {
			const auto pixel = offset + i;
			input_values[i] = source.values[pixel];
			input_x[i] = static_cast<int32_t>(pixel % width) + 1;
			input_y[i] = static_cast<int32_t>(pixel / width) + 1;
			if (!source.IsValid(pixel)) {
				validity.SetInvalid(i);
			}
		}
		local.Evaluate(*local.expression, count);
		for (idx_t i = 0; i < count; i++) {
			const auto pixel = offset + i;
			valid[pixel] = source.IsValid(pixel) && local.valid[i];
			values[pixel] = local.values[i];
		}
	}

	const auto result = CreateMemLike(src, 0, GDT_Byte);
	const auto has_nodata = c.Has("nodataval") || source.has_nodata;
	ReturnBand(c, *result, type, values, valid, has_nodata, c.Double("nodataval", source.nodata));
}

enum class ExtentType { INTERSECTION, UNION, FIRST, SECOND };

ExtentType ParseExtentType(const string &name) {
	if (StringUtil::CIEquals(name, "INTERSECTION")) {
		return ExtentType::INTERSECTION;
	}
	if (StringUtil::CIEquals(name, "UNION")) {
		return ExtentType::UNION;
	}
	if (StringUtil::CIEquals(name, "FIRST")) {
		return ExtentType::FIRST;
	}
	if (StringUtil::CIEquals(name, "SECOND")) {
		return ExtentType::SECOND;
	}
	throw InvalidInputException("ST_MapAlgebra: unknown extent type '%s', expected INTERSECTION, UNION, FIRST or "
	                            "SECOND",
	                            name);
}

void TwoRasterExecute(Call &c, MapAlgebraLocalState &local) {
	auto &ds1 = c.Raster("rast1");
	auto &ds2 = c.Raster("rast2");
	auto &band1 = GetBand(ds1, c.Int("nband1", 1));
	auto &band2 = GetBand(ds2, c.Int("nband2", 1));
	string reason;
	if (!SameAlignment(ds1, ds2, reason)) {
		throw InvalidInputException("ST_MapAlgebra: the rasters must have the same alignment. %s (see ST_Resample)",
		                            reason);
	}
	const auto extent = ParseExtentType(c.String("extenttype", "INTERSECTION"));
	const auto type = c.Has("pixeltype") ? ParsePixelType(c.String("pixeltype")) : band1.GetRasterDataType();

	// Everything below is in the pixel coordinates of the first raster
	const GeoTransform gt1(ds1);
	const GeoTransform gt2(ds2);
	double offset_col;
	double offset_row;
	gt1.ToPixel(gt2.c[0], gt2.c[3], offset_col, offset_row);
	const auto offset_x = static_cast<int64_t>(std::llround(offset_col));
	const auto offset_y = static_cast<int64_t>(std::llround(offset_row));
	const int64_t width1 = ds1.GetRasterXSize();
	const int64_t height1 = ds1.GetRasterYSize();
	const int64_t width2 = ds2.GetRasterXSize();
	const int64_t height2 = ds2.GetRasterYSize();

	int64_t x0 = 0;
	int64_t y0 = 0;
	int64_t x1 = width1;
	int64_t y1 = height1;
	switch (extent) {
	case ExtentType::FIRST:
		break;
	case ExtentType::SECOND:
		x0 = offset_x;
		y0 = offset_y;
		x1 = offset_x + width2;
		y1 = offset_y + height2;
		break;
	case ExtentType::UNION:
		x0 = MinValue<int64_t>(0, offset_x);
		y0 = MinValue<int64_t>(0, offset_y);
		x1 = MaxValue(width1, offset_x + width2);
		y1 = MaxValue(height1, offset_y + height2);
		break;
	case ExtentType::INTERSECTION:
		x0 = MaxValue<int64_t>(0, offset_x);
		y0 = MaxValue<int64_t>(0, offset_y);
		x1 = MinValue(width1, offset_x + width2);
		y1 = MinValue(height1, offset_y + height2);
		break;
	}
	if (x1 <= x0 || y1 <= y0) {
		return;
	}
	const auto width = x1 - x0;
	const auto height = y1 - y0;
	if (width > 1000000 || height > 1000000 || width * height > 4000000000LL) {
		throw InvalidInputException("ST_MapAlgebra: the resulting raster would be %lld x %lld pixels, which is outside "
		                            "of the supported range",
		                            width, height);
	}

	const BandValues source1(band1, true);
	const BandValues source2(band2, true);
	const auto pixel_count = static_cast<idx_t>(width * height);
	vector<double> values(pixel_count);
	vector<bool> valid(pixel_count);

	const auto input_value1 = FlatVector::GetData<double>(local.input.data[VALUE_1]);
	const auto input_x1 = FlatVector::GetData<int32_t>(local.input.data[X_1]);
	const auto input_y1 = FlatVector::GetData<int32_t>(local.input.data[Y_1]);
	const auto input_value2 = FlatVector::GetData<double>(local.input.data[VALUE_2]);
	const auto input_x2 = FlatVector::GetData<int32_t>(local.input.data[X_2]);
	const auto input_y2 = FlatVector::GetData<int32_t>(local.input.data[Y_2]);
	bool has1[STANDARD_VECTOR_SIZE];
	bool has2[STANDARD_VECTOR_SIZE];

	const auto has_nodata_value = c.Has("nodatanodataval");
	const auto nodata_value = c.Double("nodatanodataval", 0);

	for (idx_t offset = 0; offset < pixel_count; offset += STANDARD_VECTOR_SIZE) {
		const auto count = MinValue<idx_t>(STANDARD_VECTOR_SIZE, pixel_count - offset);
		local.input.Reset();
		auto &validity1 = FlatVector::Validity(local.input.data[VALUE_1]);
		auto &validity2 = FlatVector::Validity(local.input.data[VALUE_2]);
		auto any_both = false;
		auto any_only1 = false;
		auto any_only2 = false;
		for (idx_t i = 0; i < count; i++) {
			const auto pixel = static_cast<int64_t>(offset + i);
			const auto px1 = x0 + pixel % width;
			const auto py1 = y0 + pixel / width;
			const auto px2 = px1 - offset_x;
			const auto py2 = py1 - offset_y;
			input_x1[i] = static_cast<int32_t>(px1 + 1);
			input_y1[i] = static_cast<int32_t>(py1 + 1);
			input_x2[i] = static_cast<int32_t>(px2 + 1);
			input_y2[i] = static_cast<int32_t>(py2 + 1);

			has1[i] = px1 >= 0 && py1 >= 0 && px1 < width1 && py1 < height1 &&
			          source1.IsValid(static_cast<idx_t>(py1 * width1 + px1));
			has2[i] = px2 >= 0 && py2 >= 0 && px2 < width2 && py2 < height2 &&
			          source2.IsValid(static_cast<idx_t>(py2 * width2 + px2));
			input_value1[i] = has1[i] ? source1.values[static_cast<idx_t>(py1 * width1 + px1)] : 0;
			input_value2[i] = has2[i] ? source2.values[static_cast<idx_t>(py2 * width2 + px2)] : 0;
			if (!has1[i]) {
				validity1.SetInvalid(i);
			}
			if (!has2[i]) {
				validity2.SetInvalid(i);
			}
			any_both = any_both || (has1[i] && has2[i]);
			any_only1 = any_only1 || (has1[i] && !has2[i]);
			any_only2 = any_only2 || (!has1[i] && has2[i]);
		}

		for (idx_t i = 0; i < count; i++) {
			valid[offset + i] = false;
			values[offset + i] = 0;
			if (!has1[i] && !has2[i] && has_nodata_value) {
				valid[offset + i] = true;
				values[offset + i] = nodata_value;
			}
		}
		if (any_both) {
			local.Evaluate(*local.expression, count);
			for (idx_t i = 0; i < count; i++) {
				if (has1[i] && has2[i]) {
					valid[offset + i] = local.valid[i];
					values[offset + i] = local.values[i];
				}
			}
		}
		// The first raster has no value here: the expression for that case only sees the second raster
		if (any_only2 && local.nodata1) {
			local.Evaluate(*local.nodata1, count);
			for (idx_t i = 0; i < count; i++) {
				if (!has1[i] && has2[i]) {
					valid[offset + i] = local.valid[i];
					values[offset + i] = local.values[i];
				}
			}
		}
		if (any_only1 && local.nodata2) {
			local.Evaluate(*local.nodata2, count);
			for (idx_t i = 0; i < count; i++) {
				if (has1[i] && !has2[i]) {
					valid[offset + i] = local.valid[i];
					values[offset + i] = local.values[i];
				}
			}
		}
	}

	const auto result = CreateMemRaster(static_cast<int>(width), static_cast<int>(height), 0, GDT_Byte);
	GeoTransform gt = gt1;
	gt1.ToWorld(static_cast<double>(x0), static_cast<double>(y0), gt.c[0], gt.c[3]);
	gt.Apply(*result);
	SetCRSText(*result, GetCRS(ds1));

	// The NODATA value of the result: the one of the first band that has one. A NODATA value given for pixels that
	// have no value in either raster is an ordinary value
	auto has_nodata = source1.has_nodata;
	auto nodata = source1.nodata;
	if (!has_nodata && source2.has_nodata) {
		has_nodata = true;
		nodata = source2.nodata;
	}
	ReturnBand(c, *result, type, values, valid, has_nodata, nodata);
}

void MapAlgebraExecute(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &bind_data = state.expr.Cast<BoundFunctionExpression>().bind_info->Cast<MapAlgebraBindData>();
	auto &local = ExecuteFunctionState::GetFunctionState(state)->Cast<MapAlgebraLocalState>();
	ExecuteCall(
	    *bind_data.params,
	    [&](Call &c) {
		    if (!local.expression) {
			    return;
		    }
		    if (bind_data.two_rasters) {
			    TwoRasterExecute(c, local);
		    } else {
			    OneRasterExecute(c, local);
		    }
	    },
	    args, state, result);
}

} // namespace

//======================================================================================================================
// Registration
//======================================================================================================================

void RegisterRasterMapAlgebraFunctions(ExtensionLoader &loader) {
	const auto RASTER = RasterType();
	const vector<Param> two_raster_options = {TextP("pixeltype", true), TextP("extenttype"), TextP("nodata1expr", true),
	                                          TextP("nodata2expr", true), DblP("nodatanodataval", true)};

	RasterFunction("ST_MapAlgebra")
	    .Custom(MapAlgebraExecute, MapAlgebraBind, MapAlgebraInit)
	    .AddOptional({RastP(), IntP("nband"), TextP("pixeltype", true), TextP("expression")}, {DblP("nodataval", true)},
	                 RASTER, nullptr)
	    .AddOptional({RastP(), TextP("pixeltype", true), TextP("expression")}, {DblP("nodataval", true)}, RASTER,
	                 nullptr)
	    .AddOptional({RastP("rast1"), IntP("nband1"), RastP("rast2"), IntP("nband2"), TextP("expression")},
	                 two_raster_options, RASTER, nullptr)
	    .AddOptional({RastP("rast1"), RastP("rast2"), TextP("expression")}, two_raster_options, RASTER, nullptr)
	    .Describe(R"(
		Computes a new single-band raster pixel by pixel from one band, or from one band of each of two rasters, with a SQL expression.

		One raster: `expression` is evaluated for every pixel of band `nband` (1-based, default 1) that is not NODATA. It may use `[rast]` (or `[rast.val]`, the pixel value, a DOUBLE) and `[rast.x]` / `[rast.y]` (the 1-based column and row, INTEGER). Pixels that are NODATA in the input, and pixels for which the expression is NULL, are NODATA in the result. The NODATA value of the result is `nodataval`, by default the one of the input band, or the smallest value of the pixel type if there is none and one is needed.

		Two rasters: the rasters must have the same alignment (see ST_SameAlignment and ST_Resample). `expression` may use `[rast1]`, `[rast1.x]`, `[rast1.y]`, `[rast2]`, `[rast2.x]` and `[rast2.y]`, and is evaluated where both rasters have a value. `extenttype` is the extent of the result: `INTERSECTION` (the default), `UNION`, `FIRST` or `SECOND`. Where only the second raster has a value, the result is `nodata1expr` (an expression, NODATA if omitted); where only the first one has a value, `nodata2expr`; where neither has, the constant `nodatanodataval`. Returns NULL if the extent is empty (PostGIS returns an empty raster).

		`pixeltype` is the pixel type of the result; NULL means the type of the (first) input band. Results are rounded and clamped to the pixel type.

		The expression must be a constant string. It is parsed and bound once per query by DuckDB's own parser and binder and evaluated vectorised over the pixels, so every DuckDB scalar function and operator is available and the syntax and semantics are those of DuckDB, not of PostgreSQL. Subqueries, window functions and aggregates are not allowed. The callback (`regprocedure`) variants of PostGIS are not available.
	)",
	              R"(
		SELECT ST_DumpValues(ST_MapAlgebra(ST_AddBand(ST_MakeEmptyRaster(3, 2, 0, 0, 1), '8BUI', 10), 1, '16BSI', '[rast] * 2 + [rast.x] - [rast.y]'));
		----
		[[20.0, 21.0, 22.0], [19.0, 20.0, 21.0]]
	)")
	    .Register(loader);
}

} // namespace raster
} // namespace duckdb
