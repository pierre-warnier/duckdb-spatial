#pragma once

#include "duckdb/common/common.hpp"
#include "duckdb/common/unordered_map.hpp"
#include "duckdb/common/vector.hpp"

#include <limits>

namespace duckdb {

namespace topology {

struct TopoPoint {
	double x;
	double y;

	bool operator==(const TopoPoint &other) const {
		return x == other.x && y == other.y;
	}
	bool operator!=(const TopoPoint &other) const {
		return !(*this == other);
	}
};

struct TopoBox {
	double xmin = std::numeric_limits<double>::infinity();
	double ymin = std::numeric_limits<double>::infinity();
	double xmax = -std::numeric_limits<double>::infinity();
	double ymax = -std::numeric_limits<double>::infinity();

	bool IsEmpty() const {
		return xmin > xmax;
	}
	void Add(const TopoPoint &p) {
		xmin = MinValue(xmin, p.x);
		ymin = MinValue(ymin, p.y);
		xmax = MaxValue(xmax, p.x);
		ymax = MaxValue(ymax, p.y);
	}
	void Add(const vector<TopoPoint> &pts) {
		for (auto &p : pts) {
			Add(p);
		}
	}
	void Merge(const TopoBox &other) {
		xmin = MinValue(xmin, other.xmin);
		ymin = MinValue(ymin, other.ymin);
		xmax = MaxValue(xmax, other.xmax);
		ymax = MaxValue(ymax, other.ymax);
	}
	void Expand(double distance) {
		xmin -= distance;
		ymin -= distance;
		xmax += distance;
		ymax += distance;
	}
	bool Contains(const TopoPoint &p) const {
		return p.x >= xmin && p.x <= xmax && p.y >= ymin && p.y <= ymax;
	}
	bool Contains(const TopoBox &other) const {
		return other.xmin >= xmin && other.xmax <= xmax && other.ymin >= ymin && other.ymax <= ymax;
	}
	bool Intersects(const TopoBox &other) const {
		return other.xmin <= xmax && other.xmax >= xmin && other.ymin <= ymax && other.ymax >= ymin;
	}
	bool operator==(const TopoBox &other) const {
		return xmin == other.xmin && ymin == other.ymin && xmax == other.xmax && ymax == other.ymax;
	}
};

//! Integral values without a fractional part, anything else with the shortest exact representation
string FormatNumber(double value);

struct TopoNode {
	int32_t id = 0;
	bool has_face = false;
	int32_t face = 0;
	TopoPoint pt {0, 0};
};

struct TopoEdge {
	int32_t id = 0;
	int32_t start_node = 0;
	int32_t end_node = 0;
	int32_t next_left = 0;
	int32_t next_right = 0;
	int32_t left_face = 0;
	int32_t right_face = 0;
	vector<TopoPoint> pts;

	bool IsClosed() const {
		return start_node == end_node;
	}
	bool SameLinks(const TopoEdge &other) const {
		return next_left == other.next_left && next_right == other.next_right && left_face == other.left_face &&
		       right_face == other.right_face;
	}
};

struct TopoFace {
	int32_t id = 0;
	bool has_mbr = false;
	TopoBox mbr;
};

using EdgeMap = unordered_map<int32_t, TopoEdge>;

//! One of the two extremities of an edge, as seen from the node it is attached to
struct EdgeEnd {
	int32_t edge;
	bool outgoing;
	double angle;

	int32_t Signed() const {
		return outgoing ? edge : -edge;
	}
};

//! Direction (radians, counter-clockwise from the X axis) in which the edge leaves its start (outgoing) or end node
double EdgeEndAngle(const TopoEdge &edge, bool outgoing);

//! Sort the edge ends around a node counter-clockwise
void SortFan(vector<EdgeEnd> &fan);

//! For a fan sorted with SortFan, call set(edge_id, is_next_left, value) for every edge end: the ring walk that reaches
//! the node through an edge end continues on the first edge end found clockwise from it
template <class SET>
void LinkFan(const vector<EdgeEnd> &fan, SET &&set) {
	const auto count = fan.size();
	for (idx_t i = 0; i < count; i++) {
		auto &arriving = fan[i];
		auto &leaving = fan[(i + count - 1) % count];
		set(arriving.edge, !arriving.outgoing, leaving.Signed());
	}
}

//! Walk a ring following next_left_edge (positive ids) and next_right_edge (negative ids).
//! Returns false if the walk does not come back to the starting edge within limit steps
template <class GET>
bool WalkRing(GET &&get_edge, int32_t start, idx_t limit, vector<int32_t> &ring) {
	ring.clear();
	auto current = start;
	do {
		if (ring.size() >= limit) {
			return false;
		}
		ring.push_back(current);
		const TopoEdge *edge = get_edge(current < 0 ? -current : current);
		if (!edge) {
			return false;
		}
		current = current > 0 ? edge->next_left : edge->next_right;
		if (current == 0) {
			return false;
		}
	} while (current != start);
	return true;
}

template <class GET>
vector<TopoPoint> RingCoordinates(GET &&get_edge, const vector<int32_t> &ring) {
	vector<TopoPoint> result;
	for (auto signed_id : ring) {
		const TopoEdge *edge = get_edge(signed_id < 0 ? -signed_id : signed_id);
		auto &pts = edge->pts;
		const auto count = pts.size();
		for (idx_t i = 0; i < count; i++) {
			auto &p = signed_id > 0 ? pts[i] : pts[count - 1 - i];
			if (result.empty() || result.back() != p) {
				result.push_back(p);
			}
		}
	}
	return result;
}

double SignedArea(const vector<TopoPoint> &ring);

//! Number of times a horizontal ray going right from p crosses the polyline (half-open rule, so that the parity summed
//! over the pieces of a closed boundary tells whether p is inside)
idx_t CrossingCount(const vector<TopoPoint> &line, const TopoPoint &p);

inline bool PointInRing(const vector<TopoPoint> &ring, const TopoPoint &p) {
	return (CrossingCount(ring, p) & 1) == 1;
}

//! A point strictly inside the edge (never one of its end nodes)
TopoPoint InteriorPoint(const vector<TopoPoint> &line);

double PointSegmentDistance(const TopoPoint &p, const TopoPoint &a, const TopoPoint &b);

//! Distance from p to the polyline; segment receives the index of the closest segment
double PointLineDistance(const vector<TopoPoint> &line, const TopoPoint &p, idx_t *segment = nullptr);

//! Whether p lies on the polyline. On success segment is the index of the segment holding p and at_vertex tells
//! whether p coincides with vertex `segment` (or the last vertex when segment == size - 1)
bool LocatePointOnLine(const vector<TopoPoint> &line, const TopoPoint &p, idx_t &segment, bool &at_vertex);

struct RingInfo {
	vector<int32_t> edges;
	vector<TopoPoint> coords;
	double area = 0;
	TopoBox box;
	idx_t component = 0;
	//! The ring with the face on its left outside of it (hole or boundary of an isolated component)
	bool outer = false;
	bool closed = true;
};

struct GraphAnalysis {
	//! next_left_edge / next_right_edge implied by the geometry, indexed like the edge vector
	vector<int32_t> next_left;
	vector<int32_t> next_right;
	vector<RingInfo> rings;
	//! Index into rings of the left and right side of every edge
	vector<idx_t> left_ring;
	vector<idx_t> right_ring;
	idx_t component_count = 0;
};

//! Compute edge linking and rings of a complete set of edges from its geometry alone
void AnalyzeGraph(const vector<TopoEdge> &edges, GraphAnalysis &result);

//! Build the rings by following the given links instead of the geometric ones
void BuildRings(const vector<TopoEdge> &edges, const vector<int32_t> &next_left, const vector<int32_t> &next_right,
                GraphAnalysis &result);

} // namespace topology

} // namespace duckdb
