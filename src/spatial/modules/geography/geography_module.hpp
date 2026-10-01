#pragma once

namespace duckdb {

class ExtensionLoader;
struct LogicalType;

struct GeographyType {
	static constexpr auto NAME = "GEOG";

	//! A geometry whose vertices are (longitude, latitude) in degrees on WGS84 and whose edges are geodesics.
	//! Stored like a GEOMETRY, but as a BLOB under its own name rather than as a GEOMETRY with a CRS, so that it
	//! keeps its type in databases of every storage version and never binds to the planar functions.
	static LogicalType Get();
};

void RegisterGeographyModule(ExtensionLoader &loader);

} // namespace duckdb
