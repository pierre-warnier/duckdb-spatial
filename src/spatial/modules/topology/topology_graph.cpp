#include "spatial/modules/topology/topology_graph.hpp"

#include "duckdb/common/algorithm.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/types/value.hpp"

#include <cmath>

namespace duckdb {

namespace topology {

string FormatNumber(double value) {
	if (value == std::floor(value) && std::fabs(value) < 1e15) {
		return std::to_string(static_cast<int64_t>(value));
	}
	return Value::DOUBLE(value).ToString();
}

double EdgeEndAngle(const TopoEdge &edge, bool outgoing) {
	auto &pts = edge.pts;
	const auto count = pts.size();
	if (count < 2) {
		return 0;
	}
	if (outgoing) {
		for (idx_t i = 1; i < count; i++) {
			if (pts[i] != pts[0]) {
				return std::atan2(pts[i].y - pts[0].y, pts[i].x - pts[0].x);
			}
		}
	} else {
		auto &last = pts[count - 1];
		for (idx_t i = count - 1; i > 0; i--) {
			if (pts[i - 1] != last) {
				return std::atan2(pts[i - 1].y - last.y, pts[i - 1].x - last.x);
			}
		}
	}
	return 0;
}

void SortFan(vector<EdgeEnd> &fan) {
	std::sort(fan.begin(), fan.end(), [](const EdgeEnd &a, const EdgeEnd &b) {
		if (a.angle != b.angle) {
			return a.angle < b.angle;
		}
		if (a.edge != b.edge) {
			return a.edge < b.edge;
		}
		return a.outgoing && !b.outgoing;
	});
}

double SignedArea(const vector<TopoPoint> &ring) {
	const auto count = ring.size();
	if (count < 3) {
		return 0;
	}
	// Shift to the first vertex to keep precision with large coordinates
	auto &origin = ring[0];
	double sum = 0;
	for (idx_t i = 1; i + 1 < count; i++) {
		sum +=
		    (ring[i].x - origin.x) * (ring[i + 1].y - origin.y) - (ring[i + 1].x - origin.x) * (ring[i].y - origin.y);
	}
	return sum / 2;
}

idx_t CrossingCount(const vector<TopoPoint> &line, const TopoPoint &p) {
	idx_t crossings = 0;
	for (idx_t i = 0; i + 1 < line.size(); i++) {
		auto &a = line[i];
		auto &b = line[i + 1];
		if ((a.y > p.y) != (b.y > p.y)) {
			const auto x = a.x + (p.y - a.y) * (b.x - a.x) / (b.y - a.y);
			if (p.x < x) {
				crossings++;
			}
		}
	}
	return crossings;
}

TopoPoint InteriorPoint(const vector<TopoPoint> &line) {
	for (idx_t i = 0; i + 1 < line.size(); i++) {
		if (line[i] != line[i + 1]) {
			return TopoPoint {(line[i].x + line[i + 1].x) / 2, (line[i].y + line[i + 1].y) / 2};
		}
	}
	return line.empty() ? TopoPoint {0, 0} : line[0];
}

double PointSegmentDistance(const TopoPoint &p, const TopoPoint &a, const TopoPoint &b) {
	const auto dx = b.x - a.x;
	const auto dy = b.y - a.y;
	const auto len2 = dx * dx + dy * dy;
	if (len2 == 0) {
		return std::hypot(p.x - a.x, p.y - a.y);
	}
	auto t = ((p.x - a.x) * dx + (p.y - a.y) * dy) / len2;
	if (t <= 0) {
		return std::hypot(p.x - a.x, p.y - a.y);
	}
	if (t >= 1) {
		return std::hypot(p.x - b.x, p.y - b.y);
	}
	// Distance to the supporting line, computed from the cross product to stay accurate close to the segment
	return std::fabs((p.x - a.x) * dy - (p.y - a.y) * dx) / std::sqrt(len2);
}

double PointLineDistance(const vector<TopoPoint> &line, const TopoPoint &p, idx_t *segment) {
	double best = std::numeric_limits<double>::infinity();
	if (line.size() == 1) {
		best = std::hypot(p.x - line[0].x, p.y - line[0].y);
		if (segment) {
			*segment = 0;
		}
	}
	for (idx_t i = 0; i + 1 < line.size(); i++) {
		const auto distance = PointSegmentDistance(p, line[i], line[i + 1]);
		if (distance < best) {
			best = distance;
			if (segment) {
				*segment = i;
			}
		}
	}
	return best;
}

bool LocatePointOnLine(const vector<TopoPoint> &line, const TopoPoint &p, idx_t &segment, bool &at_vertex) {
	for (idx_t i = 0; i < line.size(); i++) {
		if (line[i] == p) {
			segment = i;
			at_vertex = true;
			return true;
		}
	}
	at_vertex = false;
	double scale = MaxValue(std::fabs(p.x), std::fabs(p.y));
	const auto tolerance = 1e-12 * MaxValue(scale, 1.0);
	idx_t closest = 0;
	const auto distance = PointLineDistance(line, p, &closest);
	if (distance > tolerance) {
		return false;
	}
	segment = closest;
	return true;
}

namespace {

struct DisjointSet {
	explicit DisjointSet(idx_t count) : parent(count) {
		for (idx_t i = 0; i < count; i++) {
			parent[i] = i;
		}
	}
	idx_t Find(idx_t i) {
		while (parent[i] != i) {
			parent[i] = parent[parent[i]];
			i = parent[i];
		}
		return i;
	}
	void Union(idx_t a, idx_t b) {
		parent[Find(a)] = Find(b);
	}
	vector<idx_t> parent;
};

} // namespace

void BuildRings(const vector<TopoEdge> &edges, const vector<int32_t> &next_left, const vector<int32_t> &next_right,
                GraphAnalysis &result) {
	const auto count = edges.size();
	unordered_map<int32_t, idx_t> edge_index;
	unordered_map<int32_t, idx_t> node_index;
	for (idx_t i = 0; i < count; i++) {
		edge_index[edges[i].id] = i;
		node_index.emplace(edges[i].start_node, node_index.size());
		node_index.emplace(edges[i].end_node, node_index.size());
	}

	DisjointSet components(node_index.size());
	for (auto &edge : edges) {
		components.Union(node_index[edge.start_node], node_index[edge.end_node]);
	}
	unordered_map<idx_t, idx_t> component_ids;

	const auto invalid = DConstants::INVALID_INDEX;
	result.rings.clear();
	result.left_ring.assign(count, invalid);
	result.right_ring.assign(count, invalid);

	for (idx_t i = 0; i < count; i++) {
		for (idx_t side = 0; side < 2; side++) {
			auto &slots = side == 0 ? result.left_ring : result.right_ring;
			if (slots[i] != invalid) {
				continue;
			}
			const auto ring_idx = result.rings.size();
			RingInfo ring;
			auto current = side == 0 ? edges[i].id : -edges[i].id;
			const auto start = current;
			while (true) {
				const auto entry = edge_index.find(current < 0 ? -current : current);
				if (entry == edge_index.end()) {
					ring.closed = false;
					break;
				}
				auto &slot = current > 0 ? result.left_ring[entry->second] : result.right_ring[entry->second];
				if (slot != invalid) {
					// Ran into a side that already belongs to a ring without coming back to the start
					ring.closed = false;
					break;
				}
				slot = ring_idx;
				ring.edges.push_back(current);
				current = current > 0 ? next_left[entry->second] : next_right[entry->second];
				if (current == start) {
					break;
				}
			}
			auto get_edge = [&](int32_t id) {
				return &edges[edge_index[id]];
			};
			ring.coords = RingCoordinates(get_edge, ring.edges);
			ring.area = SignedArea(ring.coords);
			ring.box.Add(ring.coords);
			const auto root = components.Find(node_index[edges[i].start_node]);
			const auto component = component_ids.emplace(root, component_ids.size()).first->second;
			ring.component = component;
			result.rings.push_back(std::move(ring));
		}
	}
	result.component_count = component_ids.size();

	// Within a connected component exactly one ring has the component on its right: the one with the smallest area
	vector<idx_t> outer(result.component_count, invalid);
	for (idx_t r = 0; r < result.rings.size(); r++) {
		auto &ring = result.rings[r];
		auto &best = outer[ring.component];
		if (best == invalid || ring.area < result.rings[best].area) {
			best = r;
		}
	}
	for (auto r : outer) {
		if (r != invalid) {
			result.rings[r].outer = true;
		}
	}
}

void AnalyzeGraph(const vector<TopoEdge> &edges, GraphAnalysis &result) {
	const auto count = edges.size();
	unordered_map<int32_t, vector<EdgeEnd>> fans;
	unordered_map<int32_t, idx_t> edge_index;
	for (idx_t i = 0; i < count; i++) {
		auto &edge = edges[i];
		edge_index[edge.id] = i;
		fans[edge.start_node].push_back(EdgeEnd {edge.id, true, EdgeEndAngle(edge, true)});
		fans[edge.end_node].push_back(EdgeEnd {edge.id, false, EdgeEndAngle(edge, false)});
	}
	result.next_left.assign(count, 0);
	result.next_right.assign(count, 0);
	for (auto &entry : fans) {
		auto &fan = entry.second;
		SortFan(fan);
		LinkFan(fan, [&](int32_t edge_id, bool is_left, int32_t value) {
			const auto idx = edge_index[edge_id];
			if (is_left) {
				result.next_left[idx] = value;
			} else {
				result.next_right[idx] = value;
			}
		});
	}
	BuildRings(edges, result.next_left, result.next_right, result);
}

} // namespace topology

} // namespace duckdb
