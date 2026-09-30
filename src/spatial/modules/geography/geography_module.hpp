#pragma once

namespace duckdb {

class ExtensionLoader;
struct LogicalType;

struct GeographyType {
	static constexpr auto NAME = "GEOGRAPHY";

	//! GEOMETRY in 'OGC:CRS84' (longitude, latitude in degrees on WGS84) whose edges are geodesics
	static LogicalType Get();
	static bool IsGeography(const LogicalType &type);
};

void RegisterGeographyModule(ExtensionLoader &loader);

} // namespace duckdb
