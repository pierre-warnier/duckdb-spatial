#include "spatial/modules/geography/geography_ops.hpp"

#include "duckdb/common/exception.hpp"
#include "duckdb/common/limits.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace duckdb {

namespace {

constexpr double EARTH_A = 6378137.0;
constexpr double EARTH_F = 1.0 / 298.257223563;
constexpr double EARTH_E2 = EARTH_F * (2.0 - EARTH_F);
// Mean radius, only used for the spherical step of iterations that converge on the ellipsoid
constexpr double EARTH_R = 6371008.8;
// Smallest and largest radius of curvature of the ellipsoid
constexpr double EARTH_R_MIN = EARTH_A * (1.0 - EARTH_E2);
constexpr double EARTH_R_MAX = EARTH_A / (1.0 - EARTH_F);

constexpr double DEG = 3.14159265358979323846 / 180.0;

// Difference lon2 - lon1, reduced to (-180, 180]
double LongitudeDifference(double lon1, double lon2) {
	auto diff = std::fmod(lon2 - lon1, 360.0);
	if (diff > 180.0) {
		diff -= 360.0;
	} else if (diff <= -180.0) {
		diff += 360.0;
	}
	return diff;
}

const sgl::geometry *FirstPart(const sgl::geometry &geom) {
	return geom.get_first_part();
}

void VerifyVertex(void *, const sgl::vertex_xy &vertex) {
	if (!(vertex.x >= -180.0 && vertex.x <= 180.0 && vertex.y >= -90.0 && vertex.y <= 90.0)) {
		throw InvalidInputException("GEOGRAPHY coordinates must be (longitude, latitude) in degrees, with longitude in "
		                            "[-180, 180] and latitude in [-90, 90], got (%s, %s)",
		                            std::to_string(vertex.x), std::to_string(vertex.y));
	}
}

} // namespace

//----------------------------------------------------------------------------------------------------------------------
// Setup
//----------------------------------------------------------------------------------------------------------------------

GeographyOps::GeographyOps() {
	geod_init(&geod, EARTH_A, EARTH_F);
	geod_polygon_init(&poly, 0);
}

void GeographyOps::Verify(const sgl::geometry &geom) {
	sgl::ops::visit_vertices_xy(geom, nullptr, VerifyVertex);
}

void GeographyOps::Shape::Clear() {
	segments.clear();
	rings.clear();
	polygons.clear();
	anchors.clear();
}

GeographyOps::Vec3 GeographyOps::ToCartesian(const Point &point) {
	const auto sin_lat = std::sin(point.lat * DEG);
	const auto cos_lat = std::cos(point.lat * DEG);
	const auto n = EARTH_A / std::sqrt(1.0 - EARTH_E2 * sin_lat * sin_lat);
	return {n * cos_lat * std::cos(point.lon * DEG), n * cos_lat * std::sin(point.lon * DEG),
	        n * (1.0 - EARTH_E2) * sin_lat};
}

//----------------------------------------------------------------------------------------------------------------------
// Basic geodesic problems
//----------------------------------------------------------------------------------------------------------------------

double GeographyOps::Distance(const Point &lhs, const Point &rhs) const {
	double distance = 0;
	geod_inverse(&geod, lhs.lat, lhs.lon, rhs.lat, rhs.lon, &distance, nullptr, nullptr);
	return distance;
}

double GeographyOps::Azimuth(const Point &lhs, const Point &rhs) const {
	double azimuth = 0;
	geod_inverse(&geod, lhs.lat, lhs.lon, rhs.lat, rhs.lon, nullptr, &azimuth, nullptr);
	return azimuth < 0 ? azimuth + 360.0 : azimuth;
}

GeographyOps::Point GeographyOps::Project(const Point &origin, double distance, double azimuth) const {
	Point result;
	geod_direct(&geod, origin.lat, origin.lon, azimuth, distance, &result.lat, &result.lon, nullptr);
	return result;
}

//----------------------------------------------------------------------------------------------------------------------
// Measures
//----------------------------------------------------------------------------------------------------------------------

// A ring splits the ellipsoid in two. Its interior is taken to be the smaller of the two parts, whatever its winding.
double GeographyOps::RingArea(const sgl::geometry &ring, bool &encircles_pole, bool &is_eastward) {
	encircles_pole = false;
	is_eastward = false;

	const auto vertex_count = ring.get_vertex_count();
	if (vertex_count < 4) {
		return 0;
	}

	geod_polygon_clear(&poly);
	double turn = 0;
	for (uint32_t i = 0; i + 1 < vertex_count; i++) {
		const auto vertex = ring.get_vertex_xy(i);
		const auto next = ring.get_vertex_xy(i + 1);
		geod_polygon_addpoint(&geod, &poly, vertex.y, vertex.x);
		turn += LongitudeDifference(vertex.x, next.x);
	}

	// Signed so that its magnitude is the smaller side: positive if that side is to the left of the ring
	double area = 0;
	geod_polygon_compute(&geod, &poly, 0, 1, &area, nullptr);
	const auto is_left = area >= 0;

	encircles_pole = std::fabs(turn) > 180.0;
	is_eastward = turn > 0;
	if (encircles_pole && !is_left) {
		// The interior is to the right of the ring, so it is the other pole that is inside
		is_eastward = !is_eastward;
	}
	return std::fabs(area);
}

double GeographyOps::Area(const sgl::geometry &geom) {
	switch (geom.get_type()) {
	case sgl::geometry_type::POLYGON: {
		double area = 0;
		auto ring = FirstPart(geom);
		for (uint32_t i = 0; i < geom.get_part_count(); i++) {
			bool encircles_pole;
			bool is_eastward;
			const auto ring_area = RingArea(*ring, encircles_pole, is_eastward);
			area += i == 0 ? ring_area : -ring_area;
			ring = ring->get_next();
		}
		return std::max(area, 0.0);
	}
	case sgl::geometry_type::MULTI_POLYGON:
	case sgl::geometry_type::GEOMETRY_COLLECTION: {
		double area = 0;
		auto part = FirstPart(geom);
		for (uint32_t i = 0; i < geom.get_part_count(); i++) {
			area += Area(*part);
			part = part->get_next();
		}
		return area;
	}
	default:
		return 0;
	}
}

static double PathLength(const geod_geodesic &geod, const sgl::geometry &path) {
	double length = 0;
	for (uint32_t i = 0; i + 1 < path.get_vertex_count(); i++) {
		const auto beg = path.get_vertex_xy(i);
		const auto end = path.get_vertex_xy(i + 1);
		double distance = 0;
		geod_inverse(&geod, beg.y, beg.x, end.y, end.x, &distance, nullptr, nullptr);
		length += distance;
	}
	return length;
}

double GeographyOps::Length(const sgl::geometry &geom) {
	switch (geom.get_type()) {
	case sgl::geometry_type::LINESTRING:
		return PathLength(geod, geom);
	case sgl::geometry_type::MULTI_LINESTRING:
	case sgl::geometry_type::GEOMETRY_COLLECTION: {
		double length = 0;
		auto part = FirstPart(geom);
		for (uint32_t i = 0; i < geom.get_part_count(); i++) {
			length += Length(*part);
			part = part->get_next();
		}
		return length;
	}
	default:
		return 0;
	}
}

double GeographyOps::Perimeter(const sgl::geometry &geom) {
	switch (geom.get_type()) {
	case sgl::geometry_type::POLYGON: {
		double length = 0;
		auto ring = FirstPart(geom);
		for (uint32_t i = 0; i < geom.get_part_count(); i++) {
			length += PathLength(geod, *ring);
			ring = ring->get_next();
		}
		return length;
	}
	case sgl::geometry_type::MULTI_POLYGON:
	case sgl::geometry_type::GEOMETRY_COLLECTION: {
		double length = 0;
		auto part = FirstPart(geom);
		for (uint32_t i = 0; i < geom.get_part_count(); i++) {
			length += Perimeter(*part);
			part = part->get_next();
		}
		return length;
	}
	default:
		return 0;
	}
}

//----------------------------------------------------------------------------------------------------------------------
// Distance
//----------------------------------------------------------------------------------------------------------------------

void GeographyOps::AddVertices(Shape &shape, const sgl::geometry &part) const {
	const auto vertex_count = part.get_vertex_count();
	if (vertex_count == 0) {
		return;
	}

	const auto add_segment = [&](const sgl::vertex_xy &beg, const sgl::vertex_xy &end) {
		Segment segment;
		segment.a = {beg.x, beg.y};
		segment.b = {end.x, end.y};
		segment.a3 = ToCartesian(segment.a);
		segment.b3 = ToCartesian(segment.b);
		segment.is_point = beg.x == end.x && beg.y == end.y;
		segment.has_line = false;
		segment.length = 0;

		const auto dx = segment.a3.x - segment.b3.x;
		const auto dy = segment.a3.y - segment.b3.y;
		const auto dz = segment.a3.z - segment.b3.z;
		const auto chord = std::sqrt(dx * dx + dy * dy + dz * dz);
		const auto half_angle = std::asin(std::min(1.0, chord / (2.0 * EARTH_R_MIN)));
		segment.sagitta = 1.1 * EARTH_R_MAX * (1.0 - std::cos(half_angle));

		shape.segments.push_back(segment);
	};

	const auto first = part.get_vertex_xy(0);
	shape.anchors.push_back({first.x, first.y});

	if (vertex_count == 1) {
		add_segment(first, first);
		return;
	}
	for (uint32_t i = 0; i + 1 < vertex_count; i++) {
		add_segment(part.get_vertex_xy(i), part.get_vertex_xy(i + 1));
	}
}

void GeographyOps::AddGeometry(Shape &shape, const sgl::geometry &geom) {
	switch (geom.get_type()) {
	case sgl::geometry_type::POINT:
	case sgl::geometry_type::LINESTRING:
		AddVertices(shape, geom);
		break;
	case sgl::geometry_type::POLYGON: {
		Polygon polygon;
		polygon.ring_beg = shape.rings.size();
		auto part = FirstPart(geom);
		for (uint32_t i = 0; i < geom.get_part_count(); i++) {
			// Working out which pole is inside needs the area, which is costly: only do it for rings around the axis
			double turn = 0;
			for (uint32_t k = 0; k + 1 < part->get_vertex_count(); k++) {
				turn += LongitudeDifference(part->get_vertex_xy(k).x, part->get_vertex_xy(k + 1).x);
			}
			auto encircles_pole = std::fabs(turn) > 180.0;
			auto is_eastward = false;
			if (encircles_pole) {
				RingArea(*part, encircles_pole, is_eastward);
			}

			Ring ring;
			ring.segment_beg = shape.segments.size();
			AddVertices(shape, *part);
			ring.segment_end = shape.segments.size();
			// Going east around the axis keeps the north pole to the left
			ring.contains_north_pole = encircles_pole && is_eastward;
			shape.rings.push_back(ring);

			part = part->get_next();
		}
		polygon.ring_end = shape.rings.size();
		shape.polygons.push_back(polygon);
	} break;
	case sgl::geometry_type::MULTI_POINT:
	case sgl::geometry_type::MULTI_LINESTRING:
	case sgl::geometry_type::MULTI_POLYGON:
	case sgl::geometry_type::GEOMETRY_COLLECTION: {
		auto part = FirstPart(geom);
		for (uint32_t i = 0; i < geom.get_part_count(); i++) {
			AddGeometry(shape, *part);
			part = part->get_next();
		}
	} break;
	default:
		break;
	}
}

// Distance between two straight segments in space. The geodesic distance between the corresponding edges can not be
// smaller than this minus how far each geodesic strays from its chord.
double GeographyOps::ChordDistance(const Segment &lhs, const Segment &rhs) {
	const Vec3 d1 = {lhs.b3.x - lhs.a3.x, lhs.b3.y - lhs.a3.y, lhs.b3.z - lhs.a3.z};
	const Vec3 d2 = {rhs.b3.x - rhs.a3.x, rhs.b3.y - rhs.a3.y, rhs.b3.z - rhs.a3.z};
	const Vec3 r = {lhs.a3.x - rhs.a3.x, lhs.a3.y - rhs.a3.y, lhs.a3.z - rhs.a3.z};

	const auto a = d1.x * d1.x + d1.y * d1.y + d1.z * d1.z;
	const auto e = d2.x * d2.x + d2.y * d2.y + d2.z * d2.z;
	const auto f = d2.x * r.x + d2.y * r.y + d2.z * r.z;

	double s = 0;
	double t = 0;
	constexpr double EPSILON = 1e-12;

	if (a <= EPSILON && e <= EPSILON) {
		return std::sqrt(r.x * r.x + r.y * r.y + r.z * r.z);
	}
	if (a <= EPSILON) {
		t = std::min(std::max(f / e, 0.0), 1.0);
	} else {
		const auto c = d1.x * r.x + d1.y * r.y + d1.z * r.z;
		if (e <= EPSILON) {
			s = std::min(std::max(-c / a, 0.0), 1.0);
		} else {
			const auto b = d1.x * d2.x + d1.y * d2.y + d1.z * d2.z;
			const auto denom = a * e - b * b;
			if (denom > EPSILON) {
				s = std::min(std::max((b * f - c * e) / denom, 0.0), 1.0);
			}
			t = (b * s + f) / e;
			if (t < 0) {
				t = 0;
				s = std::min(std::max(-c / a, 0.0), 1.0);
			} else if (t > 1) {
				t = 1;
				s = std::min(std::max((b - c) / a, 0.0), 1.0);
			}
		}
	}

	const auto dx = r.x + d1.x * s - d2.x * t;
	const auto dy = r.y + d1.y * s - d2.y * t;
	const auto dz = r.z + d1.z * s - d2.z * t;
	return std::sqrt(dx * dx + dy * dy + dz * dz);
}

void GeographyOps::InitLine(Segment &segment) const {
	if (segment.has_line) {
		return;
	}
	geod_inverseline(&segment.line, &geod, segment.a.lat, segment.a.lon, segment.b.lat, segment.b.lon, 0);
	segment.length = segment.line.s13;
	segment.has_line = true;
}

// Finds the foot of the perpendicular from the point on the geodesic through the segment, by repeatedly solving the
// problem on a sphere from the current estimate of the foot and moving along the geodesic by the spherical answer
// (Baselga & Martinez-Llario, 2018). The correction vanishes at the true foot, so the iteration converges on the
// ellipsoid itself.
GeographyOps::Foot GeographyOps::ProjectOnSegment(const Point &point, Segment &segment) const {
	Foot result;
	result.offset = 0;
	result.distance = std::min(Distance(point, segment.a), Distance(point, segment.b));
	if (segment.is_point) {
		return result;
	}

	InitLine(segment);

	double s = 0;
	double distance = 0;
	auto converged = false;

	for (idx_t iteration = 0; iteration < 32; iteration++) {
		double lat = 0;
		double lon = 0;
		double line_azimuth = 0;
		geod_position(&segment.line, s, &lat, &lon, &line_azimuth);

		double azimuth = 0;
		geod_inverse(&geod, lat, lon, point.lat, point.lon, &distance, &azimuth, nullptr);
		if (distance < 1e-9) {
			result.offset = 0;
			converged = true;
			break;
		}

		const auto angle = distance / EARTH_R;
		const auto turn = (azimuth - line_azimuth) * DEG;
		const auto step = EARTH_R * std::atan2(std::cos(turn) * std::sin(angle), std::cos(angle));
		result.offset = EARTH_R * std::asin(std::min(1.0, std::max(-1.0, std::sin(angle) * std::sin(turn))));

		if (std::fabs(step) < 1e-6) {
			converged = true;
			break;
		}
		s += step;
		if (!std::isfinite(s)) {
			break;
		}
	}

	if (converged && s >= 0 && s <= segment.length) {
		result.distance = std::min(result.distance, distance);
	}
	return result;
}

double GeographyOps::SegmentDistance(Segment &lhs, Segment &rhs) const {
	if (lhs.is_point && rhs.is_point) {
		return Distance(lhs.a, rhs.a);
	}
	if (lhs.is_point) {
		return ProjectOnSegment(lhs.a, rhs).distance;
	}
	if (rhs.is_point) {
		return ProjectOnSegment(rhs.a, lhs).distance;
	}

	const auto lhs_beg = ProjectOnSegment(lhs.a, rhs);
	const auto lhs_end = ProjectOnSegment(lhs.b, rhs);
	const auto rhs_beg = ProjectOnSegment(rhs.a, lhs);
	const auto rhs_end = ProjectOnSegment(rhs.b, lhs);

	const auto distance =
	    std::min(std::min(lhs_beg.distance, lhs_end.distance), std::min(rhs_beg.distance, rhs_end.distance));

	// The segments can only cross if the end points of each lie on opposite sides of the geodesic through the other.
	// That also holds when the geodesics meet on the far side of the earth, so look for the crossing itself.
	if (!(lhs_beg.offset * lhs_end.offset < 0 && rhs_beg.offset * rhs_end.offset < 0)) {
		return distance;
	}

	double lo = 0;
	double hi = lhs.length;
	auto lo_offset = lhs_beg.offset;
	auto hi_offset = lhs_end.offset;
	Foot foot = lhs_beg;
	for (idx_t iteration = 0; iteration < 64; iteration++) {
		auto s = lo - lo_offset * (hi - lo) / (hi_offset - lo_offset);
		if (!(s > lo && s < hi) || iteration % 4 == 3) {
			s = (lo + hi) / 2.0;
		}
		Point point;
		geod_position(&lhs.line, s, &point.lat, &point.lon, nullptr);
		foot = ProjectOnSegment(point, rhs);
		if (std::fabs(foot.offset) < 1e-7 || hi - lo < 1e-7) {
			break;
		}
		if ((foot.offset < 0) == (lo_offset < 0)) {
			lo = s;
			lo_offset = foot.offset;
		} else {
			hi = s;
			hi_offset = foot.offset;
		}
	}
	return foot.distance < 1e-4 ? 0 : distance;
}

// Counts the edges of the ring that cross the meridian of the point north of it
bool GeographyOps::RingContains(Shape &shape, const Ring &ring, const Point &point) const {
	auto inside = ring.contains_north_pole;

	for (auto i = ring.segment_beg; i < ring.segment_end; i++) {
		auto &segment = shape.segments[i];
		if (segment.is_point) {
			continue;
		}

		const auto span = LongitudeDifference(segment.a.lon, segment.b.lon);
		const auto target = LongitudeDifference(segment.a.lon, point.lon);
		// The western end of an edge belongs to it, the eastern end does not, so that a meridian through a vertex
		// is counted once
		if (span > 0 ? !(target >= 0 && target < span) : !(target < 0 && target >= span)) {
			continue;
		}

		// The longitude changes monotonically along the geodesic: bisect for the point on the meridian
		InitLine(segment);
		double lo = 0;
		double hi = segment.length;
		double lat = segment.a.lat;
		for (idx_t iteration = 0; iteration < 64 && hi - lo > 1e-6; iteration++) {
			const auto mid = (lo + hi) / 2.0;
			double lon = 0;
			geod_position(&segment.line, mid, &lat, &lon, nullptr);
			const auto offset = LongitudeDifference(segment.a.lon, lon);
			if (span > 0 ? offset < target : offset > target) {
				lo = mid;
			} else {
				hi = mid;
			}
		}
		geod_position(&segment.line, (lo + hi) / 2.0, &lat, nullptr, nullptr);

		if (lat > point.lat) {
			inside = !inside;
		}
	}
	return inside;
}

bool GeographyOps::Contains(Shape &shape, const Point &point) const {
	for (auto &polygon : shape.polygons) {
		if (polygon.ring_beg == polygon.ring_end || !RingContains(shape, shape.rings[polygon.ring_beg], point)) {
			continue;
		}
		auto in_hole = false;
		for (auto i = polygon.ring_beg + 1; i < polygon.ring_end && !in_hole; i++) {
			in_hole = RingContains(shape, shape.rings[i], point);
		}
		if (!in_hole) {
			return true;
		}
	}
	return false;
}

void GeographyOps::Shape::ComputeBounds() {
	center = {0, 0, 0};
	radius = 0;
	if (segments.empty()) {
		return;
	}
	for (auto &segment : segments) {
		center.x += segment.a3.x + segment.b3.x;
		center.y += segment.a3.y + segment.b3.y;
		center.z += segment.a3.z + segment.b3.z;
	}
	const auto count = 2.0 * static_cast<double>(segments.size());
	center = {center.x / count, center.y / count, center.z / count};
	for (auto &segment : segments) {
		for (auto &point : {segment.a3, segment.b3}) {
			const auto dx = point.x - center.x;
			const auto dy = point.y - center.y;
			const auto dz = point.z - center.z;
			radius = std::max(radius, std::sqrt(dx * dx + dy * dy + dz * dz) + segment.sagitta);
		}
	}
}

// Lower bound on the distance between a segment and anything within the bounding sphere of a shape
double GeographyOps::LowerBound(const Segment &segment, const Shape &shape) {
	Segment center;
	center.a3 = shape.center;
	center.b3 = shape.center;
	return std::max(0.0, ChordDistance(segment, center) - segment.sagitta - shape.radius);
}

double GeographyOps::LowerBound(const Segment &lhs, const Segment &rhs) {
	return std::max(0.0, ChordDistance(lhs, rhs) - lhs.sagitta - rhs.sagitta);
}

void GeographyOps::SetSources(const string_t &lhs, const string_t &rhs) {
	lhs_source = lhs;
	rhs_source = rhs;
	has_sources = true;
}

// Preparing a large geometry costs far more than testing a point against it, and one side of a distance is often
// the same geometry for every row: keep its prepared form as long as the serialized geometry does not change.
void GeographyOps::PrepareShape(Shape &shape, string &key, const sgl::geometry &geom, const string_t &source) {
	static constexpr idx_t MIN_KEY_SIZE = 1024;

	const auto size = has_sources ? source.GetSize() : 0;
	if (size >= MIN_KEY_SIZE && key.size() == size && memcmp(key.data(), source.GetData(), size) == 0) {
		return;
	}
	shape.Clear();
	AddGeometry(shape, geom);
	shape.ComputeBounds();
	if (size >= MIN_KEY_SIZE) {
		key.assign(source.GetData(), size);
	} else {
		key.clear();
	}
}

bool GeographyOps::Prepare(const sgl::geometry &lhs, const sgl::geometry &rhs) {
	PrepareShape(lhs_shape, lhs_key, lhs, lhs_source);
	PrepareShape(rhs_shape, rhs_key, rhs, rhs_source);
	has_sources = false;
	return !lhs_shape.segments.empty() && !rhs_shape.segments.empty();
}

// If no edges cross, a component is either entirely inside or entirely outside of a polygon
bool GeographyOps::AnyContained() {
	for (auto &anchor : lhs_shape.anchors) {
		if (Contains(rhs_shape, anchor)) {
			return true;
		}
	}
	for (auto &anchor : rhs_shape.anchors) {
		if (Contains(lhs_shape, anchor)) {
			return true;
		}
	}
	return false;
}

double GeographyOps::Distance(const sgl::geometry &lhs, const sgl::geometry &rhs) {
	if (!Prepare(lhs, rhs)) {
		return std::numeric_limits<double>::quiet_NaN();
	}
	if (AnyContained()) {
		return 0;
	}

	// Start from a pair that is likely to be close, so that most of the other pairs can be skipped
	idx_t nearest_lhs = 0;
	auto nearest_bound = std::numeric_limits<double>::infinity();
	for (idx_t i = 0; i < lhs_shape.segments.size(); i++) {
		const auto bound = LowerBound(lhs_shape.segments[i], rhs_shape);
		if (bound < nearest_bound) {
			nearest_bound = bound;
			nearest_lhs = i;
		}
	}
	idx_t nearest_rhs = 0;
	nearest_bound = std::numeric_limits<double>::infinity();
	for (idx_t j = 0; j < rhs_shape.segments.size(); j++) {
		const auto bound = LowerBound(lhs_shape.segments[nearest_lhs], rhs_shape.segments[j]);
		if (bound < nearest_bound) {
			nearest_bound = bound;
			nearest_rhs = j;
		}
	}

	auto best = SegmentDistance(lhs_shape.segments[nearest_lhs], rhs_shape.segments[nearest_rhs]);
	for (idx_t i = 0; i < lhs_shape.segments.size() && best > 0; i++) {
		auto &lhs_segment = lhs_shape.segments[i];
		if (LowerBound(lhs_segment, rhs_shape) >= best) {
			continue;
		}
		for (idx_t j = 0; j < rhs_shape.segments.size() && best > 0; j++) {
			auto &rhs_segment = rhs_shape.segments[j];
			if (LowerBound(lhs_segment, rhs_segment) >= best) {
				continue;
			}
			best = std::min(best, SegmentDistance(lhs_segment, rhs_segment));
		}
	}
	return best;
}

bool GeographyOps::IsWithinDistance(const sgl::geometry &lhs, const sgl::geometry &rhs, double limit) {
	if (!Prepare(lhs, rhs) || std::isnan(limit)) {
		return false;
	}

	Segment lhs_center;
	lhs_center.a3 = lhs_shape.center;
	lhs_center.b3 = lhs_shape.center;
	lhs_center.sagitta = lhs_shape.radius;
	if (LowerBound(lhs_center, rhs_shape) > limit) {
		return false;
	}

	if (AnyContained()) {
		return limit >= 0;
	}
	for (auto &lhs_segment : lhs_shape.segments) {
		if (LowerBound(lhs_segment, rhs_shape) > limit) {
			continue;
		}
		for (auto &rhs_segment : rhs_shape.segments) {
			if (LowerBound(lhs_segment, rhs_segment) > limit) {
				continue;
			}
			if (SegmentDistance(lhs_segment, rhs_segment) <= limit) {
				return true;
			}
		}
	}
	return false;
}

//----------------------------------------------------------------------------------------------------------------------
// Azimuthal equidistant projection
//----------------------------------------------------------------------------------------------------------------------

namespace {

struct CenterState {
	double x = 0;
	double y = 0;
	double z = 0;
	idx_t count = 0;
};

struct PlaneState {
	const geod_geodesic *geod;
	GeographyOps::Point center;
};

} // namespace

bool GeographyOps::TryGetCenter(const sgl::geometry &geom, Point &center) const {
	CenterState state;
	sgl::ops::visit_vertices_xy(geom, &state, [](void *state_p, const sgl::vertex_xy &vertex) {
		auto &state = *static_cast<CenterState *>(state_p);
		const auto point = ToCartesian({vertex.x, vertex.y});
		state.x += point.x;
		state.y += point.y;
		state.z += point.z;
		state.count++;
	});
	if (state.count == 0) {
		return false;
	}
	const auto norm = std::sqrt(state.x * state.x + state.y * state.y);
	if (norm < 1e-6 && std::fabs(state.z) < 1e-6) {
		center = {0, 0};
		return true;
	}
	center.lon = std::atan2(state.y, state.x) / DEG;
	center.lat = std::atan2(state.z, norm * (1.0 - EARTH_E2)) / DEG;
	return true;
}

void GeographyOps::ToPlane(sgl::allocator &alloc, sgl::geometry &geom, const Point &center) const {
	PlaneState state = {&geod, center};
	sgl::ops::transform_vertices(alloc, geom, &state, [](void *state_p, sgl::vertex_xyzm &vertex) {
		auto &state = *static_cast<PlaneState *>(state_p);
		double distance = 0;
		double azimuth = 0;
		geod_inverse(state.geod, state.center.lat, state.center.lon, vertex.y, vertex.x, &distance, &azimuth,
		             nullptr);
		vertex.x = distance * std::sin(azimuth * DEG);
		vertex.y = distance * std::cos(azimuth * DEG);
	});
}

void GeographyOps::FromPlane(sgl::allocator &alloc, sgl::geometry &geom, const Point &center) const {
	PlaneState state = {&geod, center};
	sgl::ops::transform_vertices(alloc, geom, &state, [](void *state_p, sgl::vertex_xyzm &vertex) {
		auto &state = *static_cast<PlaneState *>(state_p);
		const auto distance = std::sqrt(vertex.x * vertex.x + vertex.y * vertex.y);
		const auto azimuth = std::atan2(vertex.x, vertex.y) / DEG;
		geod_direct(state.geod, state.center.lat, state.center.lon, azimuth, distance, &vertex.y, &vertex.x, nullptr);
	});
}

//----------------------------------------------------------------------------------------------------------------------
// Segmentize
//----------------------------------------------------------------------------------------------------------------------

sgl::geometry *GeographyOps::Segmentize(sgl::allocator &alloc, const sgl::geometry &geom, double max_length) const {
	const auto mem = alloc.alloc(sizeof(sgl::geometry));
	const auto result = new (mem) sgl::geometry(geom.get_type(), geom.has_z(), geom.has_m());

	if (geom.is_multi_part()) {
		auto part = FirstPart(geom);
		for (uint32_t i = 0; i < geom.get_part_count(); i++) {
			result->append_part(Segmentize(alloc, *part, max_length));
			part = part->get_next();
		}
		return result;
	}

	const auto vertex_count = geom.get_vertex_count();
	const auto vertex_width = geom.get_vertex_width();
	if (vertex_count < 2) {
		result->set_vertex_array(geom.get_vertex_array(), vertex_count);
		return result;
	}

	// Count the vertices first
	vector<uint32_t> pieces(vertex_count - 1);
	uint64_t total = 1;
	for (uint32_t i = 0; i + 1 < vertex_count; i++) {
		const auto beg = geom.get_vertex_xy(i);
		const auto end = geom.get_vertex_xy(i + 1);
		const auto length = Distance({beg.x, beg.y}, {end.x, end.y});
		const auto count = std::ceil(length / max_length);
		if (count > 1e8) {
			throw InvalidInputException("ST_Segmentize: the maximum segment length is too small for this geography");
		}
		pieces[i] = std::max<uint32_t>(1, static_cast<uint32_t>(count));
		total += pieces[i];
	}
	if (total > NumericLimits<uint32_t>::Maximum()) {
		throw InvalidInputException("ST_Segmentize: the maximum segment length is too small for this geography");
	}

	const auto array = static_cast<char *>(alloc.alloc(total * vertex_width));
	auto offset = array;
	for (uint32_t i = 0; i + 1 < vertex_count; i++) {
		const auto beg = geom.get_vertex_xyzm(i);
		const auto end = geom.get_vertex_xyzm(i + 1);

		memcpy(offset, &beg, vertex_width);
		offset += vertex_width;

		if (pieces[i] == 1) {
			continue;
		}

		geod_geodesicline line;
		geod_inverseline(&line, &geod, beg.y, beg.x, end.y, end.x, 0);
		for (uint32_t k = 1; k < pieces[i]; k++) {
			const auto fraction = static_cast<double>(k) / pieces[i];
			// The remaining ordinates (z and/or m, in storage order) are interpolated linearly
			sgl::vertex_xyzm vertex = {0, 0, beg.z + (end.z - beg.z) * fraction, beg.m + (end.m - beg.m) * fraction};
			geod_position(&line, line.s13 * fraction, &vertex.y, &vertex.x, nullptr);
			memcpy(offset, &vertex, vertex_width);
			offset += vertex_width;
		}
	}
	const auto last = geom.get_vertex_xyzm(vertex_count - 1);
	memcpy(offset, &last, vertex_width);

	result->set_vertex_array(array, static_cast<uint32_t>(total));
	return result;
}

} // namespace duckdb
