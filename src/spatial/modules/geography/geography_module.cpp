#include "spatial/modules/geography/geography_module.hpp"
#include "spatial/modules/geography/geography_ops.hpp"

#include "spatial/geometry/geometry_serialization.hpp"
#include "spatial/geometry/sgl.hpp"
#include "spatial/util/function_builder.hpp"

#include "duckdb/common/types/geometry.hpp"
#include "duckdb/common/vector_operations/generic_executor.hpp"
#include "duckdb/function/cast/default_casts.hpp"
#include "duckdb/main/extension/extension_loader.hpp"
#include "duckdb/planner/expression/bound_function_expression.hpp"

#if SPATIAL_USE_GEOS
#include "spatial/modules/geos/geos_serde.hpp"
#include "geos_c.h"
#endif

namespace duckdb {

LogicalType GeographyType::Get() {
	auto type = LogicalType::GEOMETRY("OGC:CRS84");
	type.SetAlias(NAME);
	return type;
}

bool GeographyType::IsGeography(const LogicalType &type) {
	return type.id() == LogicalTypeId::GEOMETRY && type.GetAlias() == NAME;
}

namespace {

constexpr double DEG = 3.14159265358979323846 / 180.0;

//======================================================================================================================
// Local State
//======================================================================================================================

class LocalState final : public FunctionLocalState {
public:
	explicit LocalState(ClientContext &context) : arena(BufferAllocator::Get(context)), allocator(arena) {
#if SPATIAL_USE_GEOS
		geos = GEOS_init_r();
		GEOSContext_setErrorMessageHandler_r(
		    geos, [](const char *message, void *) { throw InvalidInputException(message); }, nullptr);
#endif
	}

	~LocalState() override {
#if SPATIAL_USE_GEOS
		GEOS_finish_r(geos);
#endif
	}

	static unique_ptr<FunctionLocalState> Init(ExpressionState &state, const BoundFunctionExpression &expr,
	                                           FunctionData *bind_data) {
		return make_uniq<LocalState>(state.GetContext());
	}

	static unique_ptr<FunctionLocalState> InitCast(CastLocalStateParameters &parameters) {
		return make_uniq<LocalState>(*parameters.context);
	}

	static LocalState &ResetAndGet(ExpressionState &state) {
		auto &local_state = ExecuteFunctionState::GetFunctionState(state)->Cast<LocalState>();
		local_state.arena.Reset();
		return local_state;
	}

	void Deserialize(const string_t &blob, sgl::geometry &geom) {
		Serde::Deserialize(geom, arena, blob.GetDataUnsafe(), blob.GetSize());
	}

	string_t Serialize(Vector &vector, const sgl::geometry &geom) {
		const auto size = Serde::GetRequiredSize(geom);
		auto blob = StringVector::EmptyString(vector, size);
		Serde::Serialize(geom, blob.GetDataWriteable(), size);
		blob.Finalize();
		return blob;
	}

	//! Checks that the blob holds valid longitudes and latitudes and returns it as is
	string_t Verify(const string_t &blob) {
		sgl::geometry geom;
		Deserialize(blob, geom);
		GeographyOps::Verify(geom);
		return blob;
	}

	bool TryGetPoint(const string_t &blob, GeographyOps::Point &point) {
		sgl::geometry geom;
		Deserialize(blob, geom);
		if (geom.get_type() != sgl::geometry_type::POINT || geom.is_empty()) {
			return false;
		}
		const auto vertex = geom.get_vertex_xy(0);
		point = {vertex.x, vertex.y};
		return true;
	}

	string_t MakePoint(Vector &vector, const GeographyOps::Point &point) {
		const double coords[2] = {point.lon, point.lat};
		sgl::geometry geom(sgl::geometry_type::POINT, false, false);
		geom.set_vertex_array(coords, 1);
		return Serialize(vector, geom);
	}

	ArenaAllocator arena;
	GeometryAllocator allocator;
	GeographyOps ops;
#if SPATIAL_USE_GEOS
	GEOSContextHandle_t geos;
#endif
};

//======================================================================================================================
// Casts
//======================================================================================================================

bool GeometryToGeographyCast(Vector &source, Vector &result, idx_t count, CastParameters &parameters) {
	auto &lstate = parameters.local_state->Cast<LocalState>();
	UnaryExecutor::Execute<string_t, string_t>(source, result, count, [&](const string_t &blob) {
		lstate.arena.Reset();
		lstate.Verify(blob);
		return StringVector::AddStringOrBlob(result, blob);
	});
	return true;
}

bool VarcharToGeographyCast(Vector &source, Vector &result, idx_t count, CastParameters &parameters) {
	auto &lstate = parameters.local_state->Cast<LocalState>();
	UnaryExecutor::Execute<string_t, string_t>(source, result, count, [&](const string_t &wkt) {
		lstate.arena.Reset();
		string_t blob;
		Geometry::FromString(wkt, blob, result, true);
		return lstate.Verify(blob);
	});
	return true;
}

bool GeographyToVarcharCast(Vector &source, Vector &result, idx_t count, CastParameters &parameters) {
	UnaryExecutor::Execute<string_t, string_t>(source, result, count,
	                                           [&](const string_t &blob) { return Geometry::ToString(result, blob); });
	return true;
}

//======================================================================================================================
// Constructors and conversions
//======================================================================================================================

struct ST_GeogFromText {
	static void Execute(DataChunk &args, ExpressionState &state, Vector &result) {
		auto &lstate = LocalState::ResetAndGet(state);
		UnaryExecutor::Execute<string_t, string_t>(args.data[0], result, args.size(), [&](const string_t &wkt) {
			string_t blob;
			Geometry::FromString(wkt, blob, result, true);
			return lstate.Verify(blob);
		});
	}

	static void Register(ExtensionLoader &loader) {
		for (const auto name : {"ST_GeogFromText", "ST_GeogFromWKT", "ST_GeographyFromText"}) {
			FunctionBuilder::RegisterScalar(loader, name, [](ScalarFunctionBuilder &func) {
				func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
					variant.AddParameter("wkt", LogicalType::VARCHAR);
					variant.SetReturnType(GeographyType::Get());
					variant.SetInit(LocalState::Init);
					variant.SetFunction(Execute);
					variant.CanThrowErrors();
				});
				func.SetDescription(R"(
					Creates a GEOGRAPHY from its WKT representation.

					The coordinates are longitude and latitude in degrees on WGS84, in that order. Longitudes outside of [-180, 180] and latitudes outside of [-90, 90] raise an error. `ST_GeogFromText`, `ST_GeogFromWKT` and `ST_GeographyFromText` are the same function.
				)");
				func.SetExample("SELECT ST_GeogFromText('POINT(4.3517 50.8503)');");
				func.SetTag("ext", "spatial");
				func.SetTag("category", "geography");
			});
		}
	}
};

struct ST_GeogFromWKB {
	static void Execute(DataChunk &args, ExpressionState &state, Vector &result) {
		auto &lstate = LocalState::ResetAndGet(state);
		UnaryExecutor::Execute<string_t, string_t>(args.data[0], result, args.size(), [&](const string_t &wkb) {
			string_t blob;
			Geometry::FromBinary(wkb, blob, result, true);
			return lstate.Verify(blob);
		});
	}

	static void Register(ExtensionLoader &loader) {
		FunctionBuilder::RegisterScalar(loader, "ST_GeogFromWKB", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("wkb", LogicalType::BLOB);
				variant.SetReturnType(GeographyType::Get());
				variant.SetInit(LocalState::Init);
				variant.SetFunction(Execute);
				variant.CanThrowErrors();
			});
			func.SetDescription(R"(
				Creates a GEOGRAPHY from its WKB representation.

				The coordinates are longitude and latitude in degrees on WGS84, in that order. Longitudes outside of [-180, 180] and latitudes outside of [-90, 90] raise an error.
			)");
			func.SetExample("SELECT ST_GeogFromWKB(ST_AsWKB(ST_Point(4.3517, 50.8503)));");
			func.SetTag("ext", "spatial");
			func.SetTag("category", "geography");
		});
	}
};

struct ST_GeogPoint {
	static void Execute(DataChunk &args, ExpressionState &state, Vector &result) {
		auto &lstate = LocalState::ResetAndGet(state);
		BinaryExecutor::Execute<double, double, string_t>(
		    args.data[0], args.data[1], result, args.size(), [&](double lon, double lat) {
			    const double coords[2] = {lon, lat};
			    sgl::geometry geom(sgl::geometry_type::POINT, false, false);
			    geom.set_vertex_array(coords, 1);
			    GeographyOps::Verify(geom);
			    return lstate.Serialize(result, geom);
		    });
	}

	static void Register(ExtensionLoader &loader) {
		FunctionBuilder::RegisterScalar(loader, "ST_GeogPoint", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("longitude", LogicalType::DOUBLE);
				variant.AddParameter("latitude", LogicalType::DOUBLE);
				variant.SetReturnType(GeographyType::Get());
				variant.SetInit(LocalState::Init);
				variant.SetFunction(Execute);
				variant.CanThrowErrors();
			});
			func.SetDescription("Creates a GEOGRAPHY point from a longitude and a latitude in degrees on WGS84");
			func.SetExample("SELECT ST_GeogPoint(4.3517, 50.8503);");
			func.SetTag("ext", "spatial");
			func.SetTag("category", "geography");
		});
	}
};

struct ST_AsText {
	static void Execute(DataChunk &args, ExpressionState &state, Vector &result) {
		UnaryExecutor::Execute<string_t, string_t>(
		    args.data[0], result, args.size(), [&](const string_t &blob) { return Geometry::ToString(result, blob); });
	}

	static void Register(ExtensionLoader &loader) {
		FunctionBuilder::RegisterScalar(loader, "ST_AsText", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("geog", GeographyType::Get());
				variant.SetReturnType(LogicalType::VARCHAR);
				variant.SetFunction(Execute);
				variant.SetDescription("Returns the WKT representation of the geography");
				variant.SetExample("SELECT ST_AsText(ST_GeogPoint(4.3517, 50.8503));");
			});
			func.SetTag("ext", "spatial");
			func.SetTag("category", "conversion");
		});
	}
};

struct ST_AsWKB {
	static void Execute(DataChunk &args, ExpressionState &state, Vector &result) {
		Geometry::ToBinary(args.data[0], result, args.size());
	}

	static void Register(ExtensionLoader &loader) {
		FunctionBuilder::RegisterScalar(loader, "ST_AsWKB", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("geog", GeographyType::Get());
				variant.SetReturnType(LogicalType::BLOB);
				variant.SetFunction(Execute);
				variant.SetDescription("Returns the WKB representation of the geography");
				variant.SetExample("SELECT ST_AsWKB(ST_GeogPoint(4.3517, 50.8503));");
			});
			func.SetTag("ext", "spatial");
			func.SetTag("category", "conversion");
		});
	}
};

//======================================================================================================================
// Measures
//======================================================================================================================

template <double (GeographyOps::*MEASURE)(const sgl::geometry &)>
void ExecuteMeasure(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &lstate = LocalState::ResetAndGet(state);
	UnaryExecutor::Execute<string_t, double>(args.data[0], result, args.size(), [&](const string_t &blob) {
		sgl::geometry geom;
		lstate.Deserialize(blob, geom);
		return (lstate.ops.*MEASURE)(geom);
	});
}

void RegisterMeasure(ExtensionLoader &loader, const char *name, scalar_function_t function, const char *description,
                     const char *example) {
	FunctionBuilder::RegisterScalar(loader, name, [&](ScalarFunctionBuilder &func) {
		func.AddVariant([&](ScalarFunctionVariantBuilder &variant) {
			variant.AddParameter("geog", GeographyType::Get());
			variant.SetReturnType(LogicalType::DOUBLE);
			variant.SetInit(LocalState::Init);
			variant.SetFunction(function);
			variant.SetDescription(description);
			variant.SetExample(example);
		});
		func.SetTag("ext", "spatial");
		func.SetTag("category", "property");
	});
}

void RegisterMeasures(ExtensionLoader &loader) {
	RegisterMeasure(
	    loader, "ST_Area", ExecuteMeasure<&GeographyOps::Area>,
	    "Returns the area of the polygons of a geography in square meters, computed on the WGS84 ellipsoid with "
	    "geodesic edges. The interior of a ring is the smaller of the two parts it divides the earth into, whatever "
	    "its winding order.",
	    "SELECT ST_Area(ST_GeogFromText('POLYGON((4 50, 5 50, 5 51, 4 51, 4 50))'));");
	RegisterMeasure(loader, "ST_Length", ExecuteMeasure<&GeographyOps::Length>,
	                "Returns the geodesic length of the linestrings of a geography in meters, computed on the WGS84 "
	                "ellipsoid. Polygons have a length of 0, use ST_Perimeter for them.",
	                "SELECT ST_Length(ST_GeogFromText('LINESTRING(4.3517 50.8503, 4.4025 51.2194)'));");
	RegisterMeasure(loader, "ST_Perimeter", ExecuteMeasure<&GeographyOps::Perimeter>,
	                "Returns the geodesic length of the rings of the polygons of a geography in meters, computed on "
	                "the WGS84 ellipsoid",
	                "SELECT ST_Perimeter(ST_GeogFromText('POLYGON((4 50, 5 50, 5 51, 4 51, 4 50))'));");
}

//======================================================================================================================
// Relations
//======================================================================================================================

constexpr auto DISTANCE_DESCRIPTION = R"(
	The distance is the length in meters of the shortest geodesic between the two geographies on the WGS84 ellipsoid: edges are geodesics, polygons include their interior, and the result is 0 when the geographies intersect. It is exact for points and converges to well below a millimeter for edges. Every pair of edges that cannot be ruled out by a cheap bound is examined, so the cost grows with the product of the number of vertices of the two geographies.
)";

struct ST_Distance {
	static void Execute(DataChunk &args, ExpressionState &state, Vector &result) {
		auto &lstate = LocalState::ResetAndGet(state);
		BinaryExecutor::ExecuteWithNulls<string_t, string_t, double>(
		    args.data[0], args.data[1], result, args.size(),
		    [&](const string_t &lhs_blob, const string_t &rhs_blob, ValidityMask &mask, idx_t row_idx) {
			    sgl::geometry lhs;
			    sgl::geometry rhs;
			    lstate.Deserialize(lhs_blob, lhs);
			    lstate.Deserialize(rhs_blob, rhs);
			    const auto distance = lstate.ops.Distance(lhs, rhs);
			    if (std::isnan(distance)) {
				    mask.SetInvalid(row_idx);
				    return 0.0;
			    }
			    return distance;
		    });
	}

	static void Register(ExtensionLoader &loader) {
		FunctionBuilder::RegisterScalar(loader, "ST_Distance", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("geog1", GeographyType::Get());
				variant.AddParameter("geog2", GeographyType::Get());
				variant.SetReturnType(LogicalType::DOUBLE);
				variant.SetInit(LocalState::Init);
				variant.SetFunction(Execute);
				variant.SetDescription(string("Returns the distance between two geographies in meters, or NULL if "
				                              "one of them is empty.\n") +
				                       DISTANCE_DESCRIPTION);
				variant.SetExample("SELECT ST_Distance(ST_GeogPoint(4.3517, 50.8503), ST_GeogPoint(4.4025, 51.2194));");
			});
			func.SetTag("ext", "spatial");
			func.SetTag("category", "relation");
		});
	}
};

struct ST_DWithin {
	static void Execute(DataChunk &args, ExpressionState &state, Vector &result) {
		auto &lstate = LocalState::ResetAndGet(state);
		TernaryExecutor::Execute<string_t, string_t, double, bool>(
		    args.data[0], args.data[1], args.data[2], result, args.size(),
		    [&](const string_t &lhs_blob, const string_t &rhs_blob, double limit) {
			    sgl::geometry lhs;
			    sgl::geometry rhs;
			    lstate.Deserialize(lhs_blob, lhs);
			    lstate.Deserialize(rhs_blob, rhs);
			    return lstate.ops.IsWithinDistance(lhs, rhs, limit);
		    });
	}

	static void Register(ExtensionLoader &loader) {
		FunctionBuilder::RegisterScalar(loader, "ST_DWithin", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("geog1", GeographyType::Get());
				variant.AddParameter("geog2", GeographyType::Get());
				variant.AddParameter("distance", LogicalType::DOUBLE);
				variant.SetReturnType(LogicalType::BOOLEAN);
				variant.SetInit(LocalState::Init);
				variant.SetFunction(Execute);
				variant.SetDescription(string("Returns true if two geographies are within the given distance in "
				                              "meters of each other. Joins on this predicate are not accelerated "
				                              "by the spatial join operator.\n") +
				                       DISTANCE_DESCRIPTION);
				variant.SetExample(
				    "SELECT ST_DWithin(ST_GeogPoint(4.3517, 50.8503), ST_GeogPoint(4.4025, 51.2194), 50000);");
			});
			func.SetTag("ext", "spatial");
			func.SetTag("category", "relation");
		});
	}
};

struct ST_Intersects {
	static void Execute(DataChunk &args, ExpressionState &state, Vector &result) {
		auto &lstate = LocalState::ResetAndGet(state);
		BinaryExecutor::Execute<string_t, string_t, bool>(
		    args.data[0], args.data[1], result, args.size(), [&](const string_t &lhs_blob, const string_t &rhs_blob) {
			    sgl::geometry lhs;
			    sgl::geometry rhs;
			    lstate.Deserialize(lhs_blob, lhs);
			    lstate.Deserialize(rhs_blob, rhs);
			    return lstate.ops.IsWithinDistance(lhs, rhs, 0);
		    });
	}

	static void Register(ExtensionLoader &loader) {
		FunctionBuilder::RegisterScalar(loader, "ST_Intersects", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("geog1", GeographyType::Get());
				variant.AddParameter("geog2", GeographyType::Get());
				variant.SetReturnType(LogicalType::BOOLEAN);
				variant.SetInit(LocalState::Init);
				variant.SetFunction(Execute);
				variant.SetDescription("Returns true if two geographies intersect, that is if their distance on the "
				                       "WGS84 ellipsoid is 0: edges are geodesics and polygons include their "
				                       "interior. Joins on this predicate are not accelerated by the spatial join "
				                       "operator.");
				variant.SetExample("SELECT ST_Intersects(ST_GeogFromText('POLYGON((4 50, 5 50, 5 51, 4 51, 4 50))'), "
				                   "ST_GeogPoint(4.3517, 50.8503));");
			});
			func.SetTag("ext", "spatial");
			func.SetTag("category", "relation");
		});
	}
};

//======================================================================================================================
// Constructions
//======================================================================================================================

struct ST_Project {
	static void Execute(DataChunk &args, ExpressionState &state, Vector &result) {
		auto &lstate = LocalState::ResetAndGet(state);
		TernaryExecutor::Execute<string_t, double, double, string_t>(
		    args.data[0], args.data[1], args.data[2], result, args.size(),
		    [&](const string_t &blob, double distance, double azimuth) {
			    GeographyOps::Point origin;
			    if (!lstate.TryGetPoint(blob, origin)) {
				    throw InvalidInputException("ST_Project only accepts non-empty POINT geographies");
			    }
			    return lstate.MakePoint(result, lstate.ops.Project(origin, distance, azimuth / DEG));
		    });
	}

	static void Register(ExtensionLoader &loader) {
		FunctionBuilder::RegisterScalar(loader, "ST_Project", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("origin", GeographyType::Get());
				variant.AddParameter("distance", LogicalType::DOUBLE);
				variant.AddParameter("azimuth", LogicalType::DOUBLE);
				variant.SetReturnType(GeographyType::Get());
				variant.SetInit(LocalState::Init);
				variant.SetFunction(Execute);
				variant.CanThrowErrors();
				variant.SetDescription("Returns the point reached by travelling the given distance in meters from "
				                       "the origin point along the geodesic that leaves it with the given azimuth "
				                       "in radians, clockwise from north, on the WGS84 ellipsoid");
				variant.SetExample("SELECT ST_Project(ST_GeogPoint(4.3517, 50.8503), 1000, radians(90));");
			});
			func.SetTag("ext", "spatial");
			func.SetTag("category", "construction");
		});
	}
};

struct ST_Segmentize {
	static void Execute(DataChunk &args, ExpressionState &state, Vector &result) {
		auto &lstate = LocalState::ResetAndGet(state);
		BinaryExecutor::Execute<string_t, double, string_t>(
		    args.data[0], args.data[1], result, args.size(), [&](const string_t &blob, double max_length) {
			    if (!(max_length > 0)) {
				    throw InvalidInputException("ST_Segmentize: the maximum segment length must be positive");
			    }
			    sgl::geometry geom;
			    lstate.Deserialize(blob, geom);
			    return lstate.Serialize(result, *lstate.ops.Segmentize(lstate.allocator, geom, max_length));
		    });
	}

	static void Register(ExtensionLoader &loader) {
		FunctionBuilder::RegisterScalar(loader, "ST_Segmentize", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("geog", GeographyType::Get());
				variant.AddParameter("max_segment_length", LogicalType::DOUBLE);
				variant.SetReturnType(GeographyType::Get());
				variant.SetInit(LocalState::Init);
				variant.SetFunction(Execute);
				variant.CanThrowErrors();
				variant.SetDescription("Returns the geography with vertices added along its geodesic edges so that "
				                       "no edge is longer than the given length in meters. Useful before casting "
				                       "to GEOMETRY, where edges become straight lines in longitude/latitude.");
				variant.SetExample(
				    "SELECT ST_Segmentize(ST_GeogFromText('LINESTRING(4.3517 50.8503, -74.006 40.7128)'), 100000);");
			});
			func.SetTag("ext", "spatial");
			func.SetTag("category", "construction");
		});
	}
};

#if SPATIAL_USE_GEOS
struct ST_Buffer {
	struct GeosDeleter {
		GEOSContextHandle_t ctx;
		void operator()(GEOSGeometry *geom) const {
			GEOSGeom_destroy_r(ctx, geom);
		}
	};

	static string_t Buffer(LocalState &lstate, Vector &result, const string_t &blob, double distance,
	                       int32_t quad_segs, ValidityMask &mask, idx_t row_idx) {
		sgl::geometry geom;
		lstate.Deserialize(blob, geom);

		GeographyOps::Point center;
		if (!lstate.ops.TryGetCenter(geom, center)) {
			mask.SetInvalid(row_idx);
			return string_t();
		}

		lstate.ops.ToPlane(lstate.allocator, geom, center);

		const auto plane_size = Serde::GetRequiredSize(geom);
		const auto plane_data = char_ptr_cast(lstate.arena.AllocateAligned(plane_size));
		Serde::Serialize(geom, plane_data, plane_size);

		using GeosPtr = std::unique_ptr<GEOSGeometry, GeosDeleter>;
		const GeosDeleter deleter = {lstate.geos};
		const GeosPtr plane(GeosSerde::Deserialize(lstate.geos, lstate.arena, plane_data, plane_size), deleter);
		const GeosPtr buffer(GEOSBuffer_r(lstate.geos, plane.get(), distance, quad_segs), deleter);
		if (!buffer) {
			throw InvalidInputException("ST_Buffer: could not compute the buffer");
		}

		const auto buffer_size = GeosSerde::GetRequiredSize(lstate.geos, buffer.get());
		const auto buffer_data = char_ptr_cast(lstate.arena.AllocateAligned(buffer_size));
		GeosSerde::Serialize(lstate.geos, buffer.get(), buffer_data, buffer_size);

		sgl::geometry buffered;
		Serde::Deserialize(buffered, lstate.arena, buffer_data, buffer_size);
		lstate.ops.FromPlane(lstate.allocator, buffered, center);
		return lstate.Serialize(result, buffered);
	}

	static void Execute(DataChunk &args, ExpressionState &state, Vector &result) {
		auto &lstate = LocalState::ResetAndGet(state);
		BinaryExecutor::ExecuteWithNulls<string_t, double, string_t>(
		    args.data[0], args.data[1], result, args.size(),
		    [&](const string_t &blob, double distance, ValidityMask &mask, idx_t row_idx) {
			    return Buffer(lstate, result, blob, distance, 8, mask, row_idx);
		    });
	}

	static void ExecuteWithSegments(DataChunk &args, ExpressionState &state, Vector &result) {
		auto &lstate = LocalState::ResetAndGet(state);
		TernaryExecutor::ExecuteWithNulls<string_t, double, int32_t, string_t>(
		    args.data[0], args.data[1], args.data[2], result, args.size(),
		    [&](const string_t &blob, double distance, int32_t quad_segs, ValidityMask &mask, idx_t row_idx) {
			    return Buffer(lstate, result, blob, distance, quad_segs, mask, row_idx);
		    });
	}

	static void Register(ExtensionLoader &loader) {
		FunctionBuilder::RegisterScalar(loader, "ST_Buffer", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("geog", GeographyType::Get());
				variant.AddParameter("distance", LogicalType::DOUBLE);
				variant.SetReturnType(GeographyType::Get());
				variant.SetInit(LocalState::Init);
				variant.SetFunction(Execute);
				variant.CanThrowErrors();
				variant.SetDescription(DESCRIPTION);
				variant.SetExample(EXAMPLE);
			});
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("geog", GeographyType::Get());
				variant.AddParameter("distance", LogicalType::DOUBLE);
				variant.AddParameter("num_triangles", LogicalType::INTEGER);
				variant.SetReturnType(GeographyType::Get());
				variant.SetInit(LocalState::Init);
				variant.SetFunction(ExecuteWithSegments);
				variant.CanThrowErrors();
				variant.SetDescription(DESCRIPTION);
				variant.SetExample(EXAMPLE);
			});
			func.SetTag("ext", "spatial");
			func.SetTag("category", "construction");
		});
	}

	static constexpr auto DESCRIPTION = R"(
		Returns a buffer of the given distance in meters around a geography.

		The geography is projected to an azimuthal equidistant projection centered on it, buffered in the plane and projected back. Around a point the buffer is exact. For a geography that extends over a distance d from its center, the buffer distance is off by a relative error of the order of (d / 6371 km)² / 6, about 0.004% for 100 km and 0.4% for 1000 km. `num_triangles` is the number of segments used to approximate a quarter circle (8 by default). Returns NULL for an empty geography.
	)";
	static constexpr auto EXAMPLE = "SELECT ST_Buffer(ST_GeogPoint(4.3517, 50.8503), 1000);";
};

constexpr const char *ST_Buffer::DESCRIPTION;
constexpr const char *ST_Buffer::EXAMPLE;
#endif

} // namespace

//======================================================================================================================
// Register
//======================================================================================================================

void RegisterGeographyModule(ExtensionLoader &loader) {
	const auto geography_type = GeographyType::Get();
	const auto geometry_type = LogicalType::GEOMETRY();

	loader.RegisterType(GeographyType::NAME, geography_type);

	loader.RegisterCastFunction(geometry_type, geography_type,
	                            BoundCastInfo(GeometryToGeographyCast, nullptr, LocalState::InitCast));
	loader.RegisterCastFunction(geography_type, geometry_type, DefaultCasts::ReinterpretCast);
	loader.RegisterCastFunction(LogicalType::VARCHAR, geography_type,
	                            BoundCastInfo(VarcharToGeographyCast, nullptr, LocalState::InitCast));
	loader.RegisterCastFunction(geography_type, LogicalType::VARCHAR, GeographyToVarcharCast);
	loader.RegisterCastFunction(geography_type, LogicalType::BLOB, DefaultCasts::ReinterpretCast);

	ST_GeogFromText::Register(loader);
	ST_GeogFromWKB::Register(loader);
	ST_GeogPoint::Register(loader);
	ST_AsText::Register(loader);
	ST_AsWKB::Register(loader);
	RegisterMeasures(loader);
	ST_Distance::Register(loader);
	ST_DWithin::Register(loader);
	ST_Intersects::Register(loader);
	ST_Project::Register(loader);
	ST_Segmentize::Register(loader);
#if SPATIAL_USE_GEOS
	ST_Buffer::Register(loader);
#endif
}

} // namespace duckdb
