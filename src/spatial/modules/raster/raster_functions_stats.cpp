#include "spatial/modules/raster/raster_core.hpp"

#include "duckdb/function/aggregate_function.hpp"

#include <algorithm>
#include <map>

namespace duckdb {
namespace raster {

namespace {

//======================================================================================================================
// Summary statistics
//======================================================================================================================

struct SummaryStats {
	uint64_t count = 0;
	double sum = 0;
	double mean = 0;
	// Sum of the squared deviations from the mean
	double deviation = 0;
	double min = 0;
	double max = 0;

	void Merge(const SummaryStats &other) {
		if (other.count == 0) {
			return;
		}
		if (count == 0) {
			*this = other;
			return;
		}
		const auto total = static_cast<double>(count + other.count);
		const auto delta = other.mean - mean;
		const auto weight = static_cast<double>(count) * static_cast<double>(other.count) / total;
		deviation += other.deviation + delta * delta * weight;
		mean += delta * static_cast<double>(other.count) / total;
		sum += other.sum;
		min = MinValue(min, other.min);
		max = MaxValue(max, other.max);
		count += other.count;
	}

	Value ToValue() const {
		const auto has_values = count > 0;
		child_list_t<Value> children;
		children.emplace_back("count", Value::BIGINT(static_cast<int64_t>(count)));
		children.emplace_back("sum", has_values ? Value::DOUBLE(sum) : Value(LogicalType::DOUBLE));
		children.emplace_back("mean", has_values ? Value::DOUBLE(mean) : Value(LogicalType::DOUBLE));
		children.emplace_back("stddev", has_values ? Value::DOUBLE(std::sqrt(deviation / static_cast<double>(count)))
		                                           : Value(LogicalType::DOUBLE));
		children.emplace_back("min", has_values ? Value::DOUBLE(min) : Value(LogicalType::DOUBLE));
		children.emplace_back("max", has_values ? Value::DOUBLE(max) : Value(LogicalType::DOUBLE));
		return Value::STRUCT(std::move(children));
	}
};

LogicalType SummaryStatsType() {
	child_list_t<LogicalType> children;
	children.emplace_back("count", LogicalType::BIGINT);
	children.emplace_back("sum", LogicalType::DOUBLE);
	children.emplace_back("mean", LogicalType::DOUBLE);
	children.emplace_back("stddev", LogicalType::DOUBLE);
	children.emplace_back("min", LogicalType::DOUBLE);
	children.emplace_back("max", LogicalType::DOUBLE);
	return LogicalType::STRUCT(std::move(children));
}

// Two plain passes over the band are much cheaper than updating a running mean for every pixel
void AddBandStats(GDALRasterBand &band, bool exclude_nodata, SummaryStats &stats) {
	const BandValues values(band, exclude_nodata);
	SummaryStats block;
	for (idx_t i = 0; i < values.values.size(); i++) {
		if (!values.IsValid(i)) {
			continue;
		}
		const auto value = values.values[i];
		if (block.count == 0 || value < block.min) {
			block.min = value;
		}
		if (block.count == 0 || value > block.max) {
			block.max = value;
		}
		block.count++;
		block.sum += value;
	}
	if (block.count == 0) {
		return;
	}
	block.mean = block.sum / static_cast<double>(block.count);
	for (idx_t i = 0; i < values.values.size(); i++) {
		if (values.IsValid(i)) {
			const auto delta = values.values[i] - block.mean;
			block.deviation += delta * delta;
		}
	}
	stats.Merge(block);
}

// The values of the band of the call that are not NODATA
vector<double> ValidValues(Call &c) {
	const BandValues band(c.Band("rast", "nband"), c.Bool("exclude_nodata_value", true));
	vector<double> values;
	values.reserve(band.values.size());
	for (idx_t i = 0; i < band.values.size(); i++) {
		if (band.IsValid(i)) {
			values.push_back(band.values[i]);
		}
	}
	return values;
}

void SummaryStatsExecute(Call &c) {
	SummaryStats stats;
	AddBandStats(c.Band("rast", "nband"), c.Bool("exclude_nodata_value", true), stats);
	c.ReturnValue(stats.ToValue());
}

void CountExecute(Call &c) {
	const BandValues band(c.Band("rast", "nband"), c.Bool("exclude_nodata_value", true));
	int64_t count = 0;
	for (idx_t i = 0; i < band.values.size(); i++) {
		count += band.IsValid(i) ? 1 : 0;
	}
	c.Return<int64_t>(count);
}

//======================================================================================================================
// ST_Histogram
//======================================================================================================================

LogicalType HistogramBinType() {
	child_list_t<LogicalType> children;
	children.emplace_back("min", LogicalType::DOUBLE);
	children.emplace_back("max", LogicalType::DOUBLE);
	children.emplace_back("count", LogicalType::BIGINT);
	children.emplace_back("percent", LogicalType::DOUBLE);
	return LogicalType::STRUCT(std::move(children));
}

struct HistogramBin {
	double min;
	double max;
	int64_t count;
};

void HistogramExecute(Call &c) {
	const auto values = ValidValues(c);
	const auto right = c.Bool("right", false);
	vector<double> widths;
	if (c.Has("width")) {
		widths = c.DoubleList("width");
		for (const auto width : widths) {
			if (!(width > 0)) {
				throw InvalidInputException("ST_Histogram: the bin widths must be greater than 0");
			}
		}
	}
	if (values.empty()) {
		c.ReturnValue(Value::LIST(HistogramBinType(), vector<Value>()));
		return;
	}

	const auto minimum = *std::min_element(values.begin(), values.end());
	const auto maximum = *std::max_element(values.begin(), values.end());

	auto bin_count = static_cast<int64_t>(c.Int("bins", 0));
	if (bin_count <= 0) {
		const auto count = static_cast<double>(values.size());
		// Square-root choice for small samples, Sturges' formula otherwise
		bin_count = static_cast<int64_t>(count < 30 ? std::ceil(std::sqrt(count)) : std::ceil(std::log2(count) + 1));
		if (widths.size() > 1) {
			double cycle = 0;
			for (const auto width : widths) {
				cycle += width;
			}
			bin_count = MaxValue<int64_t>(static_cast<int64_t>(widths.size()),
			                              static_cast<int64_t>(std::ceil((maximum - minimum) / cycle)) *
			                                  static_cast<int64_t>(widths.size()));
		} else if (widths.size() == 1) {
			bin_count = static_cast<int64_t>(std::ceil((maximum - minimum) / widths[0]));
		}
	}
	if (maximum == minimum || bin_count < 1) {
		bin_count = 1;
	}
	if (bin_count > 1000000) {
		throw InvalidInputException("ST_Histogram: %lld bins are too many", bin_count);
	}
	if (widths.empty()) {
		widths.push_back((maximum - minimum) / static_cast<double>(bin_count));
	}

	// Bins run upwards from the minimum, or downwards from the maximum when they are closed on the right
	vector<HistogramBin> bins(static_cast<idx_t>(bin_count));
	auto edge = right ? maximum : minimum;
	for (idx_t i = 0; i < bins.size(); i++) {
		const auto width = bin_count == 1 ? maximum - minimum : widths[i % widths.size()];
		bins[i].count = 0;
		if (!right) {
			bins[i].min = edge;
			edge += width;
			bins[i].max = edge;
		} else {
			bins[i].max = edge;
			edge -= width;
			bins[i].min = edge;
		}
	}
	// The last bin always reaches the far end of the value range
	if (!right) {
		bins.back().max = MaxValue(bins.back().max, maximum);
	} else {
		bins.back().min = MinValue(bins.back().min, minimum);
	}

	int64_t total = 0;
	for (const auto value : values) {
		for (idx_t i = 0; i < bins.size(); i++) {
			const auto last = i + 1 == bins.size();
			const auto matches = right ? (value > bins[i].min || (last && value >= bins[i].min))
			                           : (value < bins[i].max || (last && value <= bins[i].max));
			if (matches) {
				bins[i].count++;
				total++;
				break;
			}
		}
	}

	vector<Value> result;
	for (const auto &bin : bins) {
		child_list_t<Value> children;
		children.emplace_back("min", Value::DOUBLE(bin.min));
		children.emplace_back("max", Value::DOUBLE(bin.max));
		children.emplace_back("count", Value::BIGINT(bin.count));
		children.emplace_back("percent", Value::DOUBLE(total > 0 ? static_cast<double>(bin.count) / total : 0));
		result.push_back(Value::STRUCT(std::move(children)));
	}
	c.ReturnValue(Value::LIST(HistogramBinType(), std::move(result)));
}

//======================================================================================================================
// ST_Quantile
//======================================================================================================================

LogicalType QuantileType() {
	child_list_t<LogicalType> children;
	children.emplace_back("quantile", LogicalType::DOUBLE);
	children.emplace_back("value", LogicalType::DOUBLE);
	return LogicalType::STRUCT(std::move(children));
}

void CheckQuantile(double quantile) {
	if (!(quantile >= 0 && quantile <= 1)) {
		throw InvalidInputException("ST_Quantile: the quantile must be between 0 and 1, got %g", quantile);
	}
}

// Linear interpolation between the closest ranks, as R (type 7) and PostGIS do. The values must be sorted
double QuantileOfSorted(const vector<double> &sorted, double quantile) {
	const auto position = static_cast<double>(sorted.size() - 1) * quantile;
	const auto lower = static_cast<idx_t>(std::floor(position));
	const auto fraction = position - static_cast<double>(lower);
	if (fraction == 0 || lower + 1 >= sorted.size()) {
		return sorted[lower];
	}
	return sorted[lower] + fraction * (sorted[lower + 1] - sorted[lower]);
}

void QuantileExecute(Call &c) {
	auto values = ValidValues(c);
	std::sort(values.begin(), values.end());
	if (c.Declares("quantile")) {
		const auto quantile = c.Double("quantile");
		CheckQuantile(quantile);
		if (!values.empty()) {
			c.Return<double>(QuantileOfSorted(values, quantile));
		}
		return;
	}

	vector<double> quantiles = {0, 0.25, 0.5, 0.75, 1};
	if (c.Has("quantiles")) {
		quantiles = c.DoubleList("quantiles");
		std::sort(quantiles.begin(), quantiles.end());
	}
	vector<Value> result;
	for (const auto quantile : quantiles) {
		child_list_t<Value> children;
		CheckQuantile(quantile);
		children.emplace_back("quantile", Value::DOUBLE(quantile));
		if (values.empty()) {
			children.emplace_back("value", Value(LogicalType::DOUBLE));
		} else {
			children.emplace_back("value", Value::DOUBLE(QuantileOfSorted(values, quantile)));
		}
		result.push_back(Value::STRUCT(std::move(children)));
	}
	c.ReturnValue(Value::LIST(QuantileType(), std::move(result)));
}

//======================================================================================================================
// ST_ValueCount
//======================================================================================================================

LogicalType ValueCountType() {
	child_list_t<LogicalType> children;
	children.emplace_back("value", LogicalType::DOUBLE);
	children.emplace_back("count", LogicalType::BIGINT);
	return LogicalType::STRUCT(std::move(children));
}

double RoundTo(double value, double roundto) {
	if (!(roundto > 0)) {
		return value;
	}
	const auto rounded = std::round(value / roundto) * roundto;
	// Avoid -0.0 as a separate key
	return rounded == 0 ? 0 : rounded;
}

void ValueCountExecute(Call &c) {
	const auto roundto = c.Double("roundto", 0);
	std::map<double, int64_t> counts;
	for (const auto value : ValidValues(c)) {
		counts[RoundTo(value, roundto)]++;
	}

	if (c.Declares("searchvalue")) {
		const auto found = counts.find(RoundTo(c.Double("searchvalue"), roundto));
		c.Return<int64_t>(found == counts.end() ? 0 : found->second);
		return;
	}

	vector<Value> result;
	const auto add = [&](double value, int64_t count) {
		child_list_t<Value> children;
		children.emplace_back("value", Value::DOUBLE(value));
		children.emplace_back("count", Value::BIGINT(count));
		result.push_back(Value::STRUCT(std::move(children)));
	};
	if (c.Has("searchvalues")) {
		for (const auto search : c.DoubleList("searchvalues")) {
			const auto key = RoundTo(search, roundto);
			const auto found = counts.find(key);
			add(key, found == counts.end() ? 0 : found->second);
		}
	} else {
		for (const auto &entry : counts) {
			add(entry.first, entry.second);
		}
	}
	c.ReturnValue(Value::LIST(ValueCountType(), std::move(result)));
}

//======================================================================================================================
// ST_SummaryStatsAgg
//======================================================================================================================

struct SummaryStatsAgg {
	static idx_t StateSize(const AggregateFunction &) {
		return sizeof(SummaryStats);
	}

	static void Initialize(const AggregateFunction &, data_ptr_t state) {
		new (state) SummaryStats();
	}

	static void Update(Vector inputs[], AggregateInputData &, idx_t input_count, Vector &state_vec, idx_t count) {
		const GDALScope scope;
		UnifiedVectorFormat raster_format;
		UnifiedVectorFormat band_format;
		UnifiedVectorFormat exclude_format;
		UnifiedVectorFormat state_format;
		inputs[0].ToUnifiedFormat(count, raster_format);
		if (input_count > 1) {
			inputs[1].ToUnifiedFormat(count, band_format);
		}
		if (input_count > 2) {
			inputs[2].ToUnifiedFormat(count, exclude_format);
		}
		state_vec.ToUnifiedFormat(count, state_format);
		const auto rasters = UnifiedVectorFormat::GetData<string_t>(raster_format);
		const auto states = UnifiedVectorFormat::GetData<SummaryStats *>(state_format);

		for (idx_t i = 0; i < count; i++) {
			const auto raster_idx = raster_format.sel->get_index(i);
			if (!raster_format.validity.RowIsValid(raster_idx)) {
				continue;
			}
			int32_t band = 1;
			auto exclude_nodata = true;
			if (input_count > 1) {
				const auto idx = band_format.sel->get_index(i);
				if (!band_format.validity.RowIsValid(idx)) {
					continue;
				}
				band = UnifiedVectorFormat::GetData<int32_t>(band_format)[idx];
			}
			if (input_count > 2) {
				const auto idx = exclude_format.sel->get_index(i);
				if (!exclude_format.validity.RowIsValid(idx)) {
					continue;
				}
				exclude_nodata = UnifiedVectorFormat::GetData<bool>(exclude_format)[idx];
			}

			CPLErrorReset();
			const auto handle = OpenRaster(rasters[raster_idx]);
			SummaryStats stats;
			AddBandStats(GetBand(*handle.dataset, band), exclude_nodata, stats);
			states[state_format.sel->get_index(i)]->Merge(stats);
		}
	}

	static void Combine(Vector &state_vec, Vector &combined, AggregateInputData &, idx_t count) {
		UnifiedVectorFormat state_format;
		state_vec.ToUnifiedFormat(count, state_format);
		const auto states = UnifiedVectorFormat::GetData<SummaryStats *>(state_format);
		const auto targets = FlatVector::GetData<SummaryStats *>(combined);
		for (idx_t i = 0; i < count; i++) {
			targets[i]->Merge(*states[state_format.sel->get_index(i)]);
		}
	}

	static void Finalize(Vector &state_vec, AggregateInputData &, Vector &result, idx_t count, idx_t offset) {
		UnifiedVectorFormat state_format;
		state_vec.ToUnifiedFormat(count, state_format);
		const auto states = UnifiedVectorFormat::GetData<SummaryStats *>(state_format);
		for (idx_t i = 0; i < count; i++) {
			result.SetValue(i + offset, states[state_format.sel->get_index(i)]->ToValue());
		}
	}
};

} // namespace

//======================================================================================================================
// Registration
//======================================================================================================================

void RegisterRasterStatisticsFunctions(ExtensionLoader &loader) {
	const auto RASTER = RasterType();
	const auto BIGINT = LogicalType::BIGINT;
	const auto DBL = LogicalType::DOUBLE;
	const auto DBL_LIST = LogicalType::LIST(DBL);
	const auto BOOL = LogicalType::BOOLEAN;

	RasterFunction("ST_SummaryStats")
	    .AddOptional({RastP()}, {IntP("nband"), BoolP("exclude_nodata_value")}, SummaryStatsType(), SummaryStatsExecute)
	    .Add({RastP(), BoolP("exclude_nodata_value")}, SummaryStatsType(), SummaryStatsExecute)
	    .Describe(R"(
		Returns the count, sum, mean, standard deviation, minimum and maximum of the pixel values of a band, as a struct `(count, sum, mean, stddev, min, max)`.

		`nband` is 1-based and defaults to 1. NODATA pixels are left out unless `exclude_nodata_value` is false. `stddev` is the population standard deviation. When no pixel is counted, `count` is 0 and the other fields are NULL.
	)",
	              R"(
		SELECT ST_SummaryStats(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(2, 1, 0, 0, 1), '8BUI', 1), 1, 1, 5));
		----
		{'count': 2, 'sum': 6.0, 'mean': 3.0, 'stddev': 2.0, 'min': 1.0, 'max': 5.0}
	)")
	    .Register(loader);

	RasterFunction("ST_Count")
	    .AddOptional({RastP()}, {IntP("nband"), BoolP("exclude_nodata_value")}, BIGINT, CountExecute)
	    .Add({RastP(), BoolP("exclude_nodata_value")}, BIGINT, CountExecute)
	    .Describe(R"(
		Returns the number of pixels of a band (1-based, default 1) that are not NODATA, or the number of all pixels if `exclude_nodata_value` is false.
	)",
	              R"(
		SELECT ST_Count(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '8BUI', 1, 0), 1, 1, NULL));
		----
		3
	)")
	    .Register(loader);

	const auto histogram = LogicalType::LIST(HistogramBinType());
	RasterFunction("ST_Histogram")
	    .AddOptional({RastP()},
	                 {IntP("nband"), BoolP("exclude_nodata_value"), IntP("bins"), Param("width", DBL_LIST, true),
	                  BoolP("right")},
	                 histogram, HistogramExecute)
	    .Add({RastP(), IntP("nband"), BoolP("exclude_nodata_value"), IntP("bins"), BoolP("right")}, histogram,
	         HistogramExecute)
	    .AddOptional({RastP(), IntP("nband"), IntP("bins")}, {Param("width", DBL_LIST, true), BoolP("right")},
	                 histogram, HistogramExecute)
	    .Add({RastP(), IntP("nband"), IntP("bins"), BoolP("right")}, histogram, HistogramExecute)
	    .Describe(R"(
		Returns the distribution of the pixel values of a band as a list of bins `(min, max, count, percent)`, where `percent` is the fraction of the counted pixels that fall in the bin. PostGIS returns a set of rows: use `UNNEST` to get the same shape.

		`nband` is 1-based and defaults to 1; NODATA pixels are left out unless `exclude_nodata_value` is false. `bins` is the number of bins; 0 (the default) chooses it from the number of values (the square root for fewer than 30 values, Sturges' formula otherwise). `width` is a list of bin widths that is repeated to cover the value range, instead of equal-width bins. Bins include their lower bound and exclude their upper bound, except the last one; with `right` set to true the bins are listed from the largest value down, exclude their lower bound and include their upper bound.
	)",
	              R"(
		SELECT UNNEST(ST_Histogram(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '8BUI', 1), 1, 1, 5), 1, 2), recursive := true);
		----
		1.0	3.0	3	0.75
		3.0	5.0	1	0.25
	)")
	    .Register(loader);

	const auto quantiles = LogicalType::LIST(QuantileType());
	RasterFunction("ST_Quantile")
	    .AddOptional({RastP()}, {IntP("nband"), BoolP("exclude_nodata_value"), Param("quantiles", DBL_LIST, true)},
	                 quantiles, QuantileExecute)
	    .Add({RastP(), IntP("nband"), Param("quantiles", DBL_LIST)}, quantiles, QuantileExecute)
	    .Add({RastP(), Param("quantiles", DBL_LIST)}, quantiles, QuantileExecute)
	    .Add({RastP(), IntP("nband"), BoolP("exclude_nodata_value"), DblP("quantile")}, DBL, QuantileExecute)
	    .Add({RastP(), IntP("nband"), DblP("quantile")}, DBL, QuantileExecute)
	    .Add({RastP(), BoolP("exclude_nodata_value"), DblP("quantile")}, DBL, QuantileExecute)
	    .Add({RastP(), DblP("quantile")}, DBL, QuantileExecute)
	    .Describe(R"(
		Returns quantiles of the pixel values of a band.

		With a single `quantile` between 0 and 1 the result is its value. With a list of `quantiles`, or without any (the quartiles 0, 0.25, 0.5, 0.75 and 1), the result is a list of structs `(quantile, value)` in ascending order; PostGIS returns a set of rows. Quantiles are interpolated linearly between the closest ranks, like `quantile_cont`. `nband` is 1-based and defaults to 1; NODATA pixels are left out unless `exclude_nodata_value` is false. The value is NULL when no pixel is counted.
	)",
	              R"(
		SELECT ST_Quantile(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '8BUI', 1), 1, 1, 5), 0.5);
		----
		1.0
	)")
	    .Register(loader);

	const auto value_counts = LogicalType::LIST(ValueCountType());
	RasterFunction("ST_ValueCount")
	    .AddOptional({RastP()},
	                 {IntP("nband"), BoolP("exclude_nodata_value"), Param("searchvalues", DBL_LIST, true),
	                  DblP("roundto")},
	                 value_counts, ValueCountExecute)
	    .AddOptional({RastP(), IntP("nband"), Param("searchvalues", DBL_LIST, true)}, {DblP("roundto")}, value_counts,
	                 ValueCountExecute)
	    .AddOptional({RastP(), Param("searchvalues", DBL_LIST, true)}, {DblP("roundto")}, value_counts,
	                 ValueCountExecute)
	    .AddOptional({RastP(), IntP("nband"), BoolP("exclude_nodata_value"), DblP("searchvalue")}, {DblP("roundto")},
	                 BIGINT, ValueCountExecute)
	    .AddOptional({RastP(), IntP("nband"), DblP("searchvalue")}, {DblP("roundto")}, BIGINT, ValueCountExecute)
	    .AddOptional({RastP(), DblP("searchvalue")}, {DblP("roundto")}, BIGINT, ValueCountExecute)
	    .Describe(R"(
		Counts how often each pixel value occurs in a band.

		Without search values the result is a list of structs `(value, count)` for every distinct value, in ascending order; with a list of `searchvalues` it has one entry per search value, in the given order, with a count of 0 for values that do not occur; PostGIS returns a set of rows. With a single `searchvalue` the result is its count. `roundto` rounds the pixel values and the search values to a multiple of it before counting (for example 0.1 or 10; 0, the default, does not round); to round without search values, pass `NULL::DOUBLE[]` as `searchvalues`, because an untyped NULL is taken for a single search value. `nband` is 1-based and defaults to 1; NODATA pixels are left out unless `exclude_nodata_value` is false.
	)",
	              R"(
		SELECT ST_ValueCount(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '8BUI', 1), 1, 1, 5));
		----
		[{'value': 1.0, 'count': 3}, {'value': 5.0, 'count': 1}]
	)")
	    .Register(loader);

	AggregateFunctionSet aggregates("ST_SummaryStatsAgg");
	vector<FunctionDescription> descriptions;
	const vector<vector<Param>> signatures = {{RastP()},
	                                          {RastP(), IntP("nband")},
	                                          {RastP(), IntP("nband"), BoolP("exclude_nodata_value")}};
	for (const auto &signature : signatures) {
		vector<LogicalType> arguments;
		FunctionDescription description;
		for (const auto &param : signature) {
			arguments.push_back(param.type);
			description.parameter_names.emplace_back(param.name);
			description.parameter_types.push_back(param.type);
		}
		description.description =
		    "Aggregate: returns the count, sum, mean, population standard deviation, minimum and maximum of the pixel "
		    "values of a band over all rasters of a group, as a struct `(count, sum, mean, stddev, min, max)`.\n\n"
		    "`nband` is 1-based and defaults to 1; NODATA pixels are left out unless `exclude_nodata_value` is false. "
		    "NULL rasters are skipped. The `sample_percent` argument of PostGIS is not available: all pixels are "
		    "always read.";
		description.examples.emplace_back("SELECT (ST_SummaryStatsAgg(rast)).mean FROM "
		                                  "ST_ReadRaster('test/data/raster/dem.tif', 8, 8);");
		descriptions.push_back(std::move(description));

		AggregateFunction function(arguments, SummaryStatsType(), SummaryStatsAgg::StateSize,
		                           SummaryStatsAgg::Initialize, SummaryStatsAgg::Update, SummaryStatsAgg::Combine,
		                           SummaryStatsAgg::Finalize, FunctionNullHandling::DEFAULT_NULL_HANDLING);
		function.SetFallible();
		aggregates.AddFunction(function);
	}
	RegisterAggregate(loader, std::move(aggregates), std::move(descriptions));
}

} // namespace raster
} // namespace duckdb
