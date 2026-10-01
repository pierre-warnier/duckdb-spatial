#pragma once

#include "spatial/geometry/geometry_serialization.hpp"
#include "spatial/geometry/sgl.hpp"
#include "sgl/robust_predicates.hpp"

#include "duckdb/common/types/geometry.hpp"
#include "duckdb/common/types/string_type.hpp"
#include "duckdb/common/unordered_map.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace duckdb {

// Testing a point against a polygon is by far the most common predicate in a join, where neither side is constant and
// nothing can be prepared. Converting or indexing the polygon for every pair costs several times more than the test
// itself, so this case is answered directly on the serialized polygon with an exact ray crossing count.


enum class PointLocation { UNKNOWN, EXTERIOR, BOUNDARY, INTERIOR };

inline PointLocation LocateInRing(const sgl::vertex_xy &point, const sgl::geometry &ring) {
	const auto count = ring.get_vertex_count();
	if (count < 4) {
		return PointLocation::UNKNOWN;
	}
	const auto array = ring.get_vertex_array();
	const auto width = ring.get_vertex_width();

	uint32_t crossings = 0;
	sgl::vertex_xy prev;
	memcpy(&prev, array, sizeof(sgl::vertex_xy));
	for (uint32_t i = 1; i < count; i++) {
		sgl::vertex_xy next;
		memcpy(&next, array + i * width, sizeof(sgl::vertex_xy));

		// Almost all edges are entirely above or entirely below the point: skip them as cheaply as possible
		if ((prev.y > point.y && next.y > point.y) || (prev.y < point.y && next.y < point.y)) {
			prev = next;
			continue;
		}

		if (next.x == point.x && next.y == point.y) {
			return PointLocation::BOUNDARY;
		}
		if (prev.y == point.y && next.y == point.y) {
			if (point.x >= std::min(prev.x, next.x) && point.x <= std::max(prev.x, next.x)) {
				return PointLocation::BOUNDARY;
			}
		} else if (!(prev.x < point.x && next.x < point.x) &&
		           ((prev.y > point.y && next.y <= point.y) || (next.y > point.y && prev.y <= point.y))) {
			const auto orientation = sgl::robust::orient2d(&prev.x, &next.x, &point.x);
			if (orientation == 0) {
				return PointLocation::BOUNDARY;
			}
			if ((orientation > 0) != (next.y < prev.y)) {
				crossings++;
			}
		}
		prev = next;
	}
	return crossings % 2 == 1 ? PointLocation::INTERIOR : PointLocation::EXTERIOR;
}

inline PointLocation LocateInPolygon(const sgl::vertex_xy &point, const sgl::geometry &polygon) {
	auto ring = polygon.get_first_part();
	for (uint32_t i = 0; i < polygon.get_part_count(); i++) {
		const auto location = LocateInRing(point, *ring);
		if (location == PointLocation::UNKNOWN || location == PointLocation::BOUNDARY) {
			return location;
		}
		// Outside of the shell, or inside of a hole
		if ((i == 0) != (location == PointLocation::INTERIOR)) {
			return PointLocation::EXTERIOR;
		}
		ring = ring->get_next();
	}
	return polygon.get_part_count() == 0 ? PointLocation::UNKNOWN : PointLocation::INTERIOR;
}

//! Indexes of polygons whose serialized form stays at the same address while the cache is in use, which is the case
//! of the build side of a spatial join. The join makes its cache current around the evaluation of its predicate, and
//! the point-in-area test then only touches the few edges of the polygon that are level with the point.
class PreparedAreaCache {
public:
	//! The predicate argument the join passes its build side as
	static constexpr idx_t STABLE_ARGUMENT = 1;

	explicit PreparedAreaCache(Allocator &allocator) : arena(allocator) {
	}

	static PreparedAreaCache *&Current() {
		static thread_local PreparedAreaCache *current = nullptr;
		return current;
	}

	PointLocation Locate(const sgl::vertex_xy &point, const string_t &area_blob) {
		auto entry = entries.find(area_blob.GetData());
		if (entry == entries.end()) {
			const auto mem = arena.AllocateAligned(sizeof(sgl::prepared_geometry));
			const auto geom = new (mem) sgl::prepared_geometry();
			Serde::DeserializePrepared(*geom, arena, area_blob.GetData(), area_blob.GetSize());
			entry = entries.emplace(area_blob.GetData(), geom).first;
		}
		const auto &area = *entry->second;
		if (area.get_type() == sgl::geometry_type::POLYGON) {
			return LocateInPolygon(point, area);
		}
		auto polygon = area.get_first_part();
		for (uint32_t i = 0; i < area.get_part_count(); i++) {
			const auto location = LocateInPolygon(point, *polygon);
			if (location != PointLocation::EXTERIOR) {
				return location;
			}
			polygon = polygon->get_next();
		}
		return area.get_part_count() == 0 ? PointLocation::UNKNOWN : PointLocation::EXTERIOR;
	}

private:
	static PointLocation LocateInPolygon(const sgl::vertex_xy &point, const sgl::geometry &polygon) {
		auto ring = polygon.get_first_part();
		for (uint32_t i = 0; i < polygon.get_part_count(); i++) {
			auto location = PointLocation::UNKNOWN;
			if (ring->is_prepared()) {
				switch (static_cast<const sgl::prepared_geometry &>(*ring).contains(point)) {
				case sgl::point_in_polygon_result::INTERIOR:
					location = PointLocation::INTERIOR;
					break;
				case sgl::point_in_polygon_result::EXTERIOR:
					location = PointLocation::EXTERIOR;
					break;
				case sgl::point_in_polygon_result::BOUNDARY:
					location = PointLocation::BOUNDARY;
					break;
				default:
					break;
				}
			} else {
				location = LocateInRing(point, *ring);
			}
			if (location == PointLocation::UNKNOWN || location == PointLocation::BOUNDARY) {
				return location;
			}
			if ((i == 0) != (location == PointLocation::INTERIOR)) {
				return PointLocation::EXTERIOR;
			}
			ring = ring->get_next();
		}
		return polygon.get_part_count() == 0 ? PointLocation::UNKNOWN : PointLocation::INTERIOR;
	}

	ArenaAllocator arena;
	unordered_map<const char *, sgl::prepared_geometry *> entries;
};

struct PreparedAreaCacheScope {
	explicit PreparedAreaCacheScope(PreparedAreaCache &cache) {
		PreparedAreaCache::Current() = &cache;
	}
	~PreparedAreaCacheScope() {
		PreparedAreaCache::Current() = nullptr;
	}
};

//! Locates a point relative to a polygon or multipolygon, or returns UNKNOWN if the arguments are anything else.
//! area_argument is the position of the area among the arguments of the predicate.
inline PointLocation LocatePointInArea(ArenaAllocator &arena, const string_t &point_blob, const string_t &area_blob,
                                       idx_t area_argument) {
	// Only the header is read: asking the geometry for its type would walk all of it
	const auto read_type = [](const string_t &blob, uint32_t &type) {
		if (blob.GetSize() < 5 || blob.GetData()[0] != 1) {
			return false;
		}
		memcpy(&type, blob.GetData() + 1, sizeof(uint32_t));
		type %= 1000;
		return true;
	};
	uint32_t point_type;
	uint32_t area_type;
	if (!read_type(point_blob, point_type) || point_type != static_cast<uint32_t>(GeometryType::POINT)) {
		return PointLocation::UNKNOWN;
	}
	if (!read_type(area_blob, area_type) || (area_type != static_cast<uint32_t>(GeometryType::POLYGON) &&
	                                         area_type != static_cast<uint32_t>(GeometryType::MULTIPOLYGON))) {
		return PointLocation::UNKNOWN;
	}

	sgl::geometry point_geom;
	Serde::Deserialize(point_geom, arena, point_blob.GetDataUnsafe(), point_blob.GetSize());
	if (point_geom.is_empty()) {
		return PointLocation::UNKNOWN;
	}
	const auto point = point_geom.get_vertex_xy(0);
	if (std::isnan(point.x) || std::isnan(point.y)) {
		return PointLocation::UNKNOWN;
	}

	const auto cache = PreparedAreaCache::Current();
	if (cache && area_argument == PreparedAreaCache::STABLE_ARGUMENT && !area_blob.IsInlined()) {
		return cache->Locate(point, area_blob);
	}

	sgl::geometry area_geom;
	Serde::Deserialize(area_geom, arena, area_blob.GetDataUnsafe(), area_blob.GetSize());
	if (area_geom.is_empty()) {
		return PointLocation::UNKNOWN;
	}

	if (area_type == static_cast<uint32_t>(GeometryType::POLYGON)) {
		return LocateInPolygon(point, area_geom);
	}
	auto polygon = area_geom.get_first_part();
	for (uint32_t i = 0; i < area_geom.get_part_count(); i++) {
		const auto location = LocateInPolygon(point, *polygon);
		if (location != PointLocation::EXTERIOR) {
			return location;
		}
		polygon = polygon->get_next();
	}
	return PointLocation::EXTERIOR;
}


} // namespace duckdb
