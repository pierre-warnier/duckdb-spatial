#pragma once

#include "spatial/geometry/sgl.hpp"

#include "duckdb/common/vector.hpp"

#include "geodesic.h"

namespace duckdb {

//! Geodesic computations on geometries whose vertices are (longitude, latitude) in degrees on the WGS84 ellipsoid and
//! whose edges are geodesics. Distances are in meters, areas in square meters.
class GeographyOps {
public:
	struct Point {
		double lon;
		double lat;
	};

	GeographyOps();

	//! Throws if a vertex is not a valid longitude/latitude
	static void Verify(const sgl::geometry &geom);

	double Area(const sgl::geometry &geom);
	double Length(const sgl::geometry &geom);
	double Perimeter(const sgl::geometry &geom);
	//! NaN if one of the geometries has no vertices
	double Distance(const sgl::geometry &lhs, const sgl::geometry &rhs);
	bool IsWithinDistance(const sgl::geometry &lhs, const sgl::geometry &rhs, double limit);

	double Distance(const Point &lhs, const Point &rhs) const;
	//! Azimuth in degrees, clockwise from north, of the geodesic from lhs to rhs at lhs
	double Azimuth(const Point &lhs, const Point &rhs) const;
	Point Project(const Point &origin, double distance, double azimuth) const;

	//! Returns false if the geometry has no vertices
	bool TryGetCenter(const sgl::geometry &geom, Point &center) const;
	//! Azimuthal equidistant projection around the center, in meters
	void ToPlane(sgl::allocator &alloc, sgl::geometry &geom, const Point &center) const;
	void FromPlane(sgl::allocator &alloc, sgl::geometry &geom, const Point &center) const;

	//! Returns a copy of the geometry in which no edge is longer than max_length
	sgl::geometry *Segmentize(sgl::allocator &alloc, const sgl::geometry &geom, double max_length) const;

private:
	struct Vec3 {
		double x;
		double y;
		double z;
	};

	struct Segment {
		Point a;
		Point b;
		Vec3 a3;
		Vec3 b3;
		//! Upper bound on how far the geodesic strays from the straight chord between its end points
		double sagitta;
		bool is_point;

		bool has_line;
		geod_geodesicline line;
		double length;
	};

	struct Ring {
		idx_t segment_beg;
		idx_t segment_end;
		bool contains_north_pole;
	};

	struct Polygon {
		idx_t ring_beg;
		idx_t ring_end;
	};

	struct Shape {
		vector<Segment> segments;
		vector<Ring> rings;
		vector<Polygon> polygons;
		//! One vertex of every connected component
		vector<Point> anchors;
		//! Sphere around all the edges
		Vec3 center;
		double radius;

		void Clear();
		void ComputeBounds();
	};

	struct Foot {
		//! Distance to the nearest point of the segment
		double distance;
		//! Signed offset from the geodesic the segment is part of, positive to its right
		double offset;
	};

	static Vec3 ToCartesian(const Point &point);
	static double ChordDistance(const Segment &lhs, const Segment &rhs);
	static double LowerBound(const Segment &segment, const Shape &shape);
	static double LowerBound(const Segment &lhs, const Segment &rhs);

	void AddVertices(Shape &shape, const sgl::geometry &part) const;
	void AddGeometry(Shape &shape, const sgl::geometry &geom);

	double RingArea(const sgl::geometry &ring, bool &encircles_pole, bool &is_eastward);
	void InitLine(Segment &segment) const;
	Foot ProjectOnSegment(const Point &point, Segment &segment) const;
	double SegmentDistance(Segment &lhs, Segment &rhs) const;
	bool RingContains(Shape &shape, const Ring &ring, const Point &point) const;
	bool Contains(Shape &shape, const Point &point) const;
	bool Prepare(const sgl::geometry &lhs, const sgl::geometry &rhs);
	bool AnyContained();

	geod_geodesic geod;
	geod_polygon poly;
	Shape lhs_shape;
	Shape rhs_shape;
};

} // namespace duckdb
