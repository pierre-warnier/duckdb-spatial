#include "spatial/modules/raster/raster_core.hpp"

#include "duckdb/function/aggregate_function.hpp"

namespace duckdb {
namespace raster {

namespace {

//======================================================================================================================
// Shared aggregate plumbing
//======================================================================================================================
// Both aggregates keep the rasters of a group and do their work when the group is complete. DuckDB moves aggregate
// states around with memcpy, so a state must not hold anything that points into itself, such as a std::string

template <class STATE>
struct RasterAggregate {
	static idx_t StateSize(const AggregateFunction &) {
		return sizeof(STATE);
	}

	static void Initialize(const AggregateFunction &, data_ptr_t state) {
		new (state) STATE();
	}

	static void Combine(Vector &state_vec, Vector &combined, AggregateInputData &aggr_input_data, idx_t count) {
		UnifiedVectorFormat state_format;
		state_vec.ToUnifiedFormat(count, state_format);
		const auto states = UnifiedVectorFormat::GetData<STATE *>(state_format);
		const auto targets = FlatVector::GetData<STATE *>(combined);
		const auto destructive = aggr_input_data.combine_type == AggregateCombineType::ALLOW_DESTRUCTIVE;
		for (idx_t i = 0; i < count; i++) {
			auto &source = *states[state_format.sel->get_index(i)];
			auto &target = *targets[i];
			if (!target.has_arguments && source.has_arguments) {
				target.CopyArguments(source);
			}
			for (auto &raster : source.rasters) {
				if (destructive) {
					target.rasters.push_back(std::move(raster));
				} else {
					target.rasters.push_back(raster);
				}
			}
			if (destructive) {
				source.rasters.clear();
			}
		}
	}

	static void Destroy(Vector &state_vec, AggregateInputData &, idx_t count) {
		UnifiedVectorFormat state_format;
		state_vec.ToUnifiedFormat(count, state_format);
		const auto states = UnifiedVectorFormat::GetData<STATE *>(state_format);
		for (idx_t i = 0; i < count; i++) {
			const auto idx = state_format.sel->get_index(i);
			if (state_format.validity.RowIsValid(idx)) {
				states[idx]->~STATE();
			}
		}
	}

	static void Finalize(Vector &state_vec, AggregateInputData &, Vector &result, idx_t count, idx_t offset) {
		const GDALScope scope;
		UnifiedVectorFormat state_format;
		state_vec.ToUnifiedFormat(count, state_format);
		const auto states = UnifiedVectorFormat::GetData<STATE *>(state_format);
		try {
			for (idx_t i = 0; i < count; i++) {
				CPLErrorReset();
				states[state_format.sel->get_index(i)]->Finalize(result, i + offset);
			}
		} catch (...) {
			// DuckDB does not destroy the states of an aggregate whose finalize failed
			for (idx_t i = 0; i < count; i++) {
				states[state_format.sel->get_index(i)]->Release();
			}
			throw;
		}
	}
};

// Reads one argument of the current row; returns false if it is NULL
struct ArgumentReader {
	vector<UnifiedVectorFormat> formats;

	ArgumentReader(Vector inputs[], idx_t input_count, idx_t count) : formats(input_count) {
		for (idx_t i = 0; i < input_count; i++) {
			inputs[i].ToUnifiedFormat(count, formats[i]);
		}
	}

	bool IsNull(idx_t col, idx_t row) const {
		return !formats[col].validity.RowIsValid(formats[col].sel->get_index(row));
	}

	template <class T>
	const T &Get(idx_t col, idx_t row) const {
		return UnifiedVectorFormat::GetData<T>(formats[col])[formats[col].sel->get_index(row)];
	}
};

struct OpenedRaster {
	RasterHandle handle;
	// Position of the upper-left pixel in the pixel grid of the first raster
	int64_t offset_x = 0;
	int64_t offset_y = 0;
};

//======================================================================================================================
// ST_Union_Agg
//======================================================================================================================

enum class UnionType { LAST, FIRST, MIN, MAX, COUNT, SUM, MEAN, RANGE };

UnionType ParseUnionType(const string &name) {
	static const char *const NAMES[] = {"LAST", "FIRST", "MIN", "MAX", "COUNT", "SUM", "MEAN", "RANGE"};
	for (idx_t i = 0; i < 8; i++) {
		if (StringUtil::CIEquals(name, NAMES[i])) {
			return static_cast<UnionType>(i);
		}
	}
	throw InvalidInputException("ST_Union_Agg: unknown union type '%s', expected LAST, FIRST, MIN, MAX, COUNT, SUM, "
	                            "MEAN or RANGE",
	                            name);
}

struct UnionState {
	vector<string> rasters;
	bool has_arguments = false;
	int32_t band = 0;
	UnionType type = UnionType::LAST;

	void Release() {
		vector<string>().swap(rasters);
	}

	void CopyArguments(const UnionState &other) {
		has_arguments = true;
		band = other.band;
		type = other.type;
	}

	void Finalize(Vector &result, idx_t row) const {
		if (rasters.empty()) {
			FlatVector::SetNull(result, row, true);
			return;
		}

		vector<OpenedRaster> opened(rasters.size());
		int64_t x0 = 0;
		int64_t y0 = 0;
		int64_t x1 = 0;
		int64_t y1 = 0;
		for (idx_t i = 0; i < rasters.size(); i++) {
			opened[i].handle = OpenRaster(rasters[i]);
			auto &ds = *opened[i].handle.dataset;
			auto &first = *opened[0].handle.dataset;
			if (i > 0) {
				string reason;
				if (!SameAlignment(first, ds, reason)) {
					throw InvalidInputException("ST_Union_Agg: all rasters must have the same alignment. %s (see "
					                            "ST_Resample)",
					                            reason);
				}
				if (band == 0 && ds.GetRasterCount() != first.GetRasterCount()) {
					throw InvalidInputException("ST_Union_Agg: all rasters must have the same number of bands, got %d "
					                            "and %d (or give a band number)",
					                            first.GetRasterCount(), ds.GetRasterCount());
				}
				double col;
				double pixel_row;
				GeoTransform(first).ToPixel(GeoTransform(ds).c[0], GeoTransform(ds).c[3], col, pixel_row);
				opened[i].offset_x = static_cast<int64_t>(std::llround(col));
				opened[i].offset_y = static_cast<int64_t>(std::llround(pixel_row));
			}
			x0 = MinValue(x0, opened[i].offset_x);
			y0 = MinValue(y0, opened[i].offset_y);
			x1 = MaxValue(x1, opened[i].offset_x + ds.GetRasterXSize());
			y1 = MaxValue(y1, opened[i].offset_y + ds.GetRasterYSize());
		}
		const auto width = x1 - x0;
		const auto height = y1 - y0;
		CheckGridSize(static_cast<double>(width), static_cast<double>(height));

		auto &first = *opened[0].handle.dataset;
		vector<int32_t> bands;
		if (band != 0) {
			bands.push_back(band);
		} else {
			for (int32_t b = 1; b <= first.GetRasterCount(); b++) {
				bands.push_back(b);
			}
		}

		const auto output = CreateMemRaster(static_cast<int>(width), static_cast<int>(height), 0, GDT_Byte);
		GeoTransform gt(first);
		GeoTransform(first).ToWorld(static_cast<double>(x0), static_cast<double>(y0), gt.c[0], gt.c[3]);
		gt.Apply(*output);
		SetCRSText(*output, GetCRS(first));

		const auto pixel_count = static_cast<idx_t>(width * height);
		for (idx_t b = 0; b < bands.size(); b++) {
			vector<double> values(pixel_count, 0);
			vector<double> minimum(type == UnionType::RANGE ? pixel_count : 0, 0);
			vector<uint32_t> counts(pixel_count, 0);

			for (auto &raster : opened) {
				auto &ds = *raster.handle.dataset;
				const BandValues source(GetBand(ds, bands[b]), true);
				for (int64_t y = 0; y < source.height; y++) {
					auto target = static_cast<idx_t>((raster.offset_y - y0 + y) * width + (raster.offset_x - x0));
					auto pixel = static_cast<idx_t>(y * source.width);
					for (int64_t x = 0; x < source.width; x++, target++, pixel++) {
						if (!source.IsValid(pixel)) {
							continue;
						}
						const auto value = source.values[pixel];
						const auto seen = counts[target] > 0;
						switch (type) {
						case UnionType::LAST:
							values[target] = value;
							break;
						case UnionType::FIRST:
							if (!seen) {
								values[target] = value;
							}
							break;
						case UnionType::MIN:
							values[target] = seen ? MinValue(values[target], value) : value;
							break;
						case UnionType::MAX:
							values[target] = seen ? MaxValue(values[target], value) : value;
							break;
						case UnionType::RANGE:
							values[target] = seen ? MaxValue(values[target], value) : value;
							minimum[target] = seen ? MinValue(minimum[target], value) : value;
							break;
						case UnionType::COUNT:
							break;
						case UnionType::SUM:
						case UnionType::MEAN:
							values[target] += value;
							break;
						}
						counts[target]++;
					}
				}
			}

			auto &first_band = GetBand(first, bands[b]);
			auto band_type = first_band.GetRasterDataType();
			double nodata = 0;
			auto has_nodata = GetNoData(first_band, nodata);
			if (type == UnionType::COUNT) {
				// Every pixel has a count, possibly 0
				band_type = GDT_UInt32;
				has_nodata = false;
				for (idx_t i = 0; i < pixel_count; i++) {
					values[i] = counts[i];
				}
			} else {
				if (type == UnionType::MEAN) {
					band_type = GDT_Float64;
				}
				auto any_empty = false;
				for (idx_t i = 0; i < pixel_count && !any_empty; i++) {
					any_empty = counts[i] == 0;
				}
				if (any_empty && !has_nodata) {
					has_nodata = true;
					nodata = MinPossibleValue(band_type);
				}
				nodata = ClampToPixelType(nodata, band_type);
				for (idx_t i = 0; i < pixel_count; i++) {
					if (counts[i] == 0) {
						values[i] = nodata;
					} else if (type == UnionType::MEAN) {
						values[i] /= counts[i];
					} else if (type == UnionType::RANGE) {
						values[i] -= minimum[i];
					}
				}
			}
			AddBand(*output, band_type);
			auto &out_band = *output->GetRasterBand(static_cast<int>(b + 1));
			SetNoData(out_band, has_nodata, nodata);
			WriteBand(out_band, values);
		}
		FlatVector::GetData<string_t>(result)[row] = SerializeRaster(*output, result);
	}
};

void UnionUpdate(Vector inputs[], AggregateInputData &, idx_t input_count, Vector &state_vec, idx_t count) {
	const ArgumentReader arguments(inputs, input_count, count);
	UnifiedVectorFormat state_format;
	state_vec.ToUnifiedFormat(count, state_format);
	const auto states = UnifiedVectorFormat::GetData<UnionState *>(state_format);

	for (idx_t i = 0; i < count; i++) {
		auto skip = false;
		for (idx_t col = 0; col < input_count; col++) {
			skip = skip || arguments.IsNull(col, i);
		}
		if (skip) {
			continue;
		}
		auto &state = *states[state_format.sel->get_index(i)];
		if (!state.has_arguments) {
			state.has_arguments = true;
			for (idx_t col = 1; col < input_count; col++) {
				if (inputs[col].GetType().id() == LogicalTypeId::INTEGER) {
					state.band = arguments.Get<int32_t>(col, i);
					if (state.band < 1) {
						throw InvalidInputException("ST_Union_Agg: band %d is out of range, bands are numbered from 1",
						                            state.band);
					}
				} else {
					state.type = ParseUnionType(arguments.Get<string_t>(col, i).GetString());
				}
			}
		}
		state.rasters.push_back(arguments.Get<string_t>(0, i).GetString());
	}
}

//======================================================================================================================
// ST_Retile
//======================================================================================================================

struct RetileState {
	vector<string> rasters;
	bool has_arguments = false;
	OGREnvelope extent;
	double scalex = 0;
	double scaley = 0;
	int32_t tile_width = 0;
	int32_t tile_height = 0;
	vector<string> algorithm;

	void Release() {
		vector<string>().swap(rasters);
		vector<string>().swap(algorithm);
	}

	void CopyArguments(const RetileState &other) {
		has_arguments = true;
		extent = other.extent;
		scalex = other.scalex;
		scaley = other.scaley;
		tile_width = other.tile_width;
		tile_height = other.tile_height;
		algorithm = other.algorithm;
	}

	void Finalize(Vector &result, idx_t row) const {
		if (rasters.empty()) {
			FlatVector::SetNull(result, row, true);
			return;
		}
		vector<RasterHandle> opened(rasters.size());
		vector<OGREnvelope> envelopes(rasters.size());
		for (idx_t i = 0; i < rasters.size(); i++) {
			opened[i] = OpenRaster(rasters[i]);
			auto &ds = *opened[i].dataset;
			auto &first = *opened[0].dataset;
			if (GetCRS(ds) != GetCRS(first)) {
				throw InvalidInputException("ST_Retile: all rasters must have the same coordinate system");
			}
			if (ds.GetRasterCount() != first.GetRasterCount() || ds.GetRasterCount() == 0 ||
			    ds.GetRasterBand(1)->GetRasterDataType() != first.GetRasterBand(1)->GetRasterDataType()) {
				throw InvalidInputException("ST_Retile: all rasters must have the same bands, and at least one");
			}
			envelopes[i] = RasterEnvelope(ds);
		}
		auto &first = *opened[0].dataset;

		const auto tile_size_x = scalex * tile_width;
		const auto tile_size_y = scaley * tile_height;
		const auto tiles_x = std::ceil((extent.MaxX - extent.MinX) / tile_size_x - 1e-9);
		const auto tiles_y = std::ceil((extent.MaxY - extent.MinY) / tile_size_y - 1e-9);
		if (!(tiles_x >= 1) || !(tiles_y >= 1) || tiles_x * tiles_y > 1e6) {
			throw InvalidInputException("ST_Retile: the extent would be cut into %.0f x %.0f tiles, which is outside "
			                            "of the supported range",
			                            tiles_x, tiles_y);
		}

		vector<Value> tiles;
		for (idx_t ty = 0; ty < static_cast<idx_t>(tiles_y); ty++) {
			for (idx_t tx = 0; tx < static_cast<idx_t>(tiles_x); tx++) {
				OGREnvelope tile_extent;
				tile_extent.MinX = extent.MinX + tx * tile_size_x;
				tile_extent.MaxX = tile_extent.MinX + tile_size_x;
				tile_extent.MaxY = extent.MaxY - ty * tile_size_y;
				tile_extent.MinY = tile_extent.MaxY - tile_size_y;

				GDALDatasetUniquePtr tile;
				for (idx_t i = 0; i < opened.size(); i++) {
					const auto &envelope = envelopes[i];
					if (envelope.MinX >= tile_extent.MaxX || envelope.MaxX <= tile_extent.MinX ||
					    envelope.MinY >= tile_extent.MaxY || envelope.MaxY <= tile_extent.MinY) {
						continue;
					}
					if (!tile) {
						tile = CreateMemRaster(tile_width, tile_height, first.GetRasterCount(),
						                       first.GetRasterBand(1)->GetRasterDataType());
						GeoTransform gt;
						gt.c[0] = tile_extent.MinX;
						gt.c[1] = scalex;
						gt.c[3] = tile_extent.MaxY;
						gt.c[5] = -scaley;
						gt.Apply(*tile);
						SetCRSText(*tile, GetCRS(first));
						// Pixels that no raster covers must be recognisable
						for (int b = 1; b <= first.GetRasterCount(); b++) {
							auto &band = *tile->GetRasterBand(b);
							double nodata;
							if (!GetNoData(*first.GetRasterBand(b), nodata)) {
								nodata = MinPossibleValue(band.GetRasterDataType());
							}
							SetNoData(band, true, nodata);
							CheckGDAL(band.Fill(nodata), "Could not initialize the band");
						}
					}
					WarpInto(*opened[i].dataset, *tile, algorithm.empty() ? "NearestNeighbour" : algorithm[0], 0.125);
				}
				if (tile) {
					tiles.push_back(RasterValue(*tile));
				}
			}
		}
		result.SetValue(row, Value::LIST(RasterType(), std::move(tiles)));
	}
};

void RetileUpdate(Vector inputs[], AggregateInputData &, idx_t input_count, Vector &state_vec, idx_t count) {
	const GDALScope scope;
	const ArgumentReader arguments(inputs, input_count, count);
	UnifiedVectorFormat state_format;
	state_vec.ToUnifiedFormat(count, state_format);
	const auto states = UnifiedVectorFormat::GetData<RetileState *>(state_format);

	for (idx_t i = 0; i < count; i++) {
		auto skip = false;
		for (idx_t col = 0; col < input_count; col++) {
			skip = skip || arguments.IsNull(col, i);
		}
		if (skip) {
			continue;
		}
		auto &state = *states[state_format.sel->get_index(i)];
		if (!state.has_arguments) {
			const auto extent = GeometryFromWKB(arguments.Get<string_t>(1, i));
			if (extent->IsEmpty()) {
				throw InvalidInputException("ST_Retile: the extent is empty");
			}
			extent->getEnvelope(&state.extent);
			state.scalex = std::fabs(arguments.Get<double>(2, i));
			state.scaley = std::fabs(arguments.Get<double>(3, i));
			state.tile_width = arguments.Get<int32_t>(4, i);
			state.tile_height = arguments.Get<int32_t>(5, i);
			if (input_count > 6) {
				state.algorithm.push_back(arguments.Get<string_t>(6, i).GetString());
			}
			if (!(state.scalex > 0) || !(state.scaley > 0) || state.tile_width <= 0 || state.tile_height <= 0 ||
			    !(state.extent.MaxX > state.extent.MinX) || !(state.extent.MaxY > state.extent.MinY)) {
				throw InvalidInputException("ST_Retile: the pixel size and the tile size must be positive and the "
				                            "extent must have an area");
			}
			state.has_arguments = true;
		}
		state.rasters.push_back(arguments.Get<string_t>(0, i).GetString());
	}
}

FunctionDescription Describe(const vector<Param> &signature, const char *description, const char *example) {
	FunctionDescription result;
	for (const auto &param : signature) {
		result.parameter_names.emplace_back(param.name);
		result.parameter_types.push_back(param.type);
	}
	result.description = description;
	result.examples.emplace_back(example);
	return result;
}

vector<LogicalType> ArgumentTypes(const vector<Param> &signature) {
	vector<LogicalType> result;
	for (const auto &param : signature) {
		result.push_back(param.type);
	}
	return result;
}

} // namespace

//======================================================================================================================
// Registration
//======================================================================================================================

void RegisterRasterAggregateFunctions(ExtensionLoader &loader) {
	const auto RASTER = RasterType();

	{
		const char *description =
		    "Aggregate: merges the rasters of a group into one raster that covers them all.\n\n"
		    "The rasters must have the same alignment (see ST_SameAlignment and ST_Resample). Where rasters overlap, "
		    "`uniontype` decides the value: `LAST` (the default), `FIRST`, `MIN`, `MAX`, `COUNT` (the number of "
		    "rasters with a value, stored as `32BUI`), `SUM`, `MEAN` (stored as `64BF`) or `RANGE` (the difference "
		    "between the largest and the smallest value). NODATA pixels do not take part; pixels that no raster gives "
		    "a value are NODATA (the NODATA value of the first raster, or the smallest value of the pixel type). "
		    "With `nband` (1-based) the result has that band only, otherwise all bands are merged and the rasters "
		    "must have the same number of bands. NULL rasters are skipped.\n\n"
		    "PostGIS calls this aggregate `ST_Union`; that name is a scalar geometry function here. `FIRST` and `LAST` "
		    "depend on the order of the rows: use `ST_Union_Agg(rast ORDER BY ...)` for a defined result. All "
		    "rasters of a group are kept in memory until the group is complete.";
		const char *example = "SELECT ST_Width(ST_Union_Agg(rast, 'MEAN')) FROM "
		                      "ST_ReadRaster('test/data/raster/dem.tif', 8, 8);";

		AggregateFunctionSet set("ST_Union_Agg");
		vector<FunctionDescription> descriptions;
		const vector<vector<Param>> signatures = {{RastP()},
		                                          {RastP(), TextP("uniontype")},
		                                          {RastP(), IntP("nband")},
		                                          {RastP(), IntP("nband"), TextP("uniontype")}};
		for (const auto &signature : signatures) {
			AggregateFunction function(ArgumentTypes(signature), RASTER, RasterAggregate<UnionState>::StateSize,
			                           RasterAggregate<UnionState>::Initialize, UnionUpdate,
			                           RasterAggregate<UnionState>::Combine, RasterAggregate<UnionState>::Finalize,
			                           FunctionNullHandling::DEFAULT_NULL_HANDLING, nullptr, nullptr,
			                           RasterAggregate<UnionState>::Destroy);
			function.SetFallible();
			set.AddFunction(function);
			descriptions.push_back(Describe(signature, description, example));
		}
		RegisterAggregate(loader, std::move(set), std::move(descriptions));
	}

	{
		const char *description =
		    "Aggregate: rebuilds the rasters of a group, a coverage tiled in any way, as a regular set of tiles, and "
		    "returns the tiles as a list in row order.\n\n"
		    "The tiles are `tw` x `th` pixels of `sfx` x `sfy` world units and start at the upper-left corner of the "
		    "bounding box of `ext`, which they cover completely (the last tiles of a row or column may extend beyond "
		    "it). Each tile is resampled with `algo` (default `NearestNeighbor`, see ST_Resample) from the rasters "
		    "that overlap it, later rasters over earlier ones; tiles that no raster overlaps are left out, and pixels "
		    "that no raster covers are NODATA. The rasters must have the same coordinate system and the same bands; "
		    "`ext` must be in that coordinate system. The arguments other than the raster are taken from the first "
		    "row of the group.\n\n"
		    "In PostGIS, ST_Retile is a set-returning function that takes the name of a table and of its raster "
		    "column. Here it is an aggregate over the rasters themselves: `SELECT UNNEST(ST_Retile(rast, ...)) FROM "
		    "coverage`. All rasters of a group are kept in memory until the group is complete.";
		const char *example = "SELECT len(ST_Retile(rast, ST_MakeEnvelope(150000, 169760, 150320, 170000), 20.0, 20.0, "
		                      "8, 6)) FROM ST_ReadRaster('test/data/raster/dem.tif', 5, 7);";

		AggregateFunctionSet set("ST_Retile");
		vector<FunctionDescription> descriptions;
		const vector<Param> base = {RastP(), GeomP("ext"), DblP("sfx"), DblP("sfy"), IntP("tw"), IntP("th")};
		auto with_algorithm = base;
		with_algorithm.push_back(TextP("algo"));
		for (const auto &signature : {base, with_algorithm}) {
			AggregateFunction function(ArgumentTypes(signature), LogicalType::LIST(RASTER),
			                           RasterAggregate<RetileState>::StateSize,
			                           RasterAggregate<RetileState>::Initialize, RetileUpdate,
			                           RasterAggregate<RetileState>::Combine, RasterAggregate<RetileState>::Finalize,
			                           FunctionNullHandling::DEFAULT_NULL_HANDLING, nullptr, nullptr,
			                           RasterAggregate<RetileState>::Destroy);
			function.SetFallible();
			set.AddFunction(function);
			descriptions.push_back(Describe(signature, description, example));
		}
		RegisterAggregate(loader, std::move(set), std::move(descriptions));
	}
}

} // namespace raster
} // namespace duckdb
