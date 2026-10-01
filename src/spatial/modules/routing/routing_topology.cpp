#include "spatial/modules/routing/routing_module.hpp"
#include "spatial/modules/routing/routing_common.hpp"

#include "sgl/robust_predicates.hpp"

#include "duckdb/common/exception.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <unordered_map>

namespace duckdb {

namespace routing {

namespace {

//----------------------------------------------------------------------------------------------------------------------
// Geometry helpers
//----------------------------------------------------------------------------------------------------------------------
struct Point2 {
	double x;
	double y;

	bool operator==(const Point2 &other) const {
		return x == other.x && y == other.y;
	}
	bool operator!=(const Point2 &other) const {
		return !(*this == other);
	}
	bool operator<(const Point2 &other) const {
		return x != other.x ? x < other.x : y < other.y;
	}
};

double Distance(const Point2 &a, const Point2 &b) {
	return std::hypot(b.x - a.x, b.y - a.y);
}

class WKBReader {
public:
	explicit WKBReader(const string &blob_p) : blob(blob_p) {
	}

	//! Reads the XY coordinates of a POINT or a LINESTRING, returns the base geometry type (0 when unreadable)
	uint32_t Read(vector<Point2> &points) {
		points.clear();
		if (blob.size() < 5) {
			return 0;
		}
		swap = blob[0] == 0;
		offset = 1;
		auto type = ReadInt();
		idx_t width = 2;
		if (type & 0x80000000) {
			width++;
		}
		if (type & 0x40000000) {
			width++;
		}
		if (type & 0x20000000) {
			offset += 4;
		}
		type &= 0x1FFFFFFF;
		const auto dimensions = type / 1000;
		type %= 1000;
		if (dimensions == 1 || dimensions == 2) {
			width = 3;
		} else if (dimensions == 3) {
			width = 4;
		}
		idx_t count = 1;
		if (type == 2) {
			if (offset + 4 > blob.size()) {
				return 0;
			}
			count = ReadInt();
		} else if (type != 1) {
			return type;
		}
		if (offset + count * width * sizeof(double) > blob.size()) {
			return 0;
		}
		for (idx_t i = 0; i < count; i++) {
			Point2 point;
			point.x = ReadDouble();
			point.y = ReadDouble();
			offset += (width - 2) * sizeof(double);
			if (type == 1 && std::isnan(point.x) && std::isnan(point.y)) {
				continue;
			}
			points.push_back(point);
		}
		return type;
	}

private:
	template <class T>
	T ReadRaw() {
		char buffer[sizeof(T)];
		memcpy(buffer, blob.data() + offset, sizeof(T));
		if (swap) {
			std::reverse(buffer, buffer + sizeof(T));
		}
		T value;
		memcpy(&value, buffer, sizeof(T));
		offset += sizeof(T);
		return value;
	}
	uint32_t ReadInt() {
		return ReadRaw<uint32_t>();
	}
	double ReadDouble() {
		return ReadRaw<double>();
	}

	const string &blob;
	idx_t offset = 0;
	bool swap = false;
};

string WriteWKB(uint32_t type, const Point2 *points, idx_t count) {
	string result;
	result.reserve(9 + count * 16);
	result.push_back(1);
	result.append(const_char_ptr_cast(&type), sizeof(uint32_t));
	if (type == 2) {
		const auto size = UnsafeNumericCast<uint32_t>(count);
		result.append(const_char_ptr_cast(&size), sizeof(uint32_t));
	}
	for (idx_t i = 0; i < count; i++) {
		result.append(const_char_ptr_cast(&points[i].x), sizeof(double));
		result.append(const_char_ptr_cast(&points[i].y), sizeof(double));
	}
	return result;
}

string WritePoint(const Point2 &point) {
	return WriteWKB(1, &point, 1);
}

struct Line {
	int64_t id = 0;
	int64_t source = 0;
	int64_t target = 0;
	vector<Point2> points;

	bool HasGeometry() const {
		return points.size() >= 2;
	}
	bool Closed() const {
		return HasGeometry() && points.front() == points.back();
	}
};

void ReadLinePoints(const string &function_name, const InputColumn &column, idx_t row, vector<Point2> &points) {
	points.clear();
	if (!column.validity.empty() && !column.validity[row]) {
		return;
	}
	const auto type = WKBReader(column.blobs[row]).Read(points);
	if (type != 2) {
		throw InvalidInputException("%s: column \"%s\" must contain LINESTRING geometries", function_name, column.name);
	}
	for (auto &point : points) {
		if (!std::isfinite(point.x) || !std::isfinite(point.y)) {
			throw InvalidInputException("%s: column \"%s\" contains a non-finite coordinate", function_name,
			                            column.name);
		}
	}
}

bool ReadPoint(const string &function_name, const InputColumn &column, idx_t row, Point2 &point) {
	if (!column.validity.empty() && !column.validity[row]) {
		return false;
	}
	vector<Point2> points;
	const auto type = WKBReader(column.blobs[row]).Read(points);
	if (type != 1) {
		throw InvalidInputException("%s: column \"%s\" must contain POINT geometries", function_name, column.name);
	}
	if (points.empty()) {
		return false;
	}
	point = points[0];
	return true;
}

//! Lines sorted by identifier, so that results do not depend on the order in which the rows arrive
vector<Line> ReadLines(const string &function_name, const RoutingInput &input, idx_t id_column, idx_t geom_column) {
	vector<Line> lines(input.row_count);
	for (idx_t i = 0; i < input.row_count; i++) {
		lines[i].id = input.Get(id_column).integers[i];
		ReadLinePoints(function_name, input.Get(geom_column), i, lines[i].points);
	}
	std::sort(lines.begin(), lines.end(), [](const Line &a, const Line &b) {
		if (a.id != b.id) {
			return a.id < b.id;
		}
		return a.points < b.points;
	});
	return lines;
}

//----------------------------------------------------------------------------------------------------------------------
// Segment intersection
//----------------------------------------------------------------------------------------------------------------------
double Orient(const Point2 &a, const Point2 &b, const Point2 &c) {
	return sgl::robust::orient2d(&a.x, &b.x, &c.x);
}

bool InBox(const Point2 &a, const Point2 &b, const Point2 &p) {
	return p.x >= MinValue(a.x, b.x) && p.x <= MaxValue(a.x, b.x) && p.y >= MinValue(a.y, b.y) &&
	       p.y <= MaxValue(a.y, b.y);
}

struct Intersection {
	idx_t count = 0;
	Point2 points[2];
	bool overlap = false;

	void Add(const Point2 &point) {
		for (idx_t i = 0; i < count; i++) {
			if (points[i] == point) {
				return;
			}
		}
		if (count < 2) {
			points[count++] = point;
		}
	}
};

//! Intersection of the segments ab and cd. Touching end points are returned with their exact coordinates.
Intersection Intersect(const Point2 &a, const Point2 &b, const Point2 &c, const Point2 &d) {
	Intersection result;
	const auto o1 = Orient(a, b, c);
	const auto o2 = Orient(a, b, d);
	const auto o3 = Orient(c, d, a);
	const auto o4 = Orient(c, d, b);
	if (o1 == 0 && o2 == 0 && o3 == 0 && o4 == 0) {
		if (InBox(a, b, c)) {
			result.Add(c);
		}
		if (InBox(a, b, d)) {
			result.Add(d);
		}
		if (InBox(c, d, a)) {
			result.Add(a);
		}
		if (InBox(c, d, b)) {
			result.Add(b);
		}
		result.overlap = result.count == 2;
		return result;
	}
	if ((o1 > 0 && o2 > 0) || (o1 < 0 && o2 < 0) || (o3 > 0 && o4 > 0) || (o3 < 0 && o4 < 0)) {
		return result;
	}
	if (o1 == 0) {
		if (InBox(a, b, c)) {
			result.Add(c);
		}
	} else if (o2 == 0) {
		if (InBox(a, b, d)) {
			result.Add(d);
		}
	} else if (o3 == 0) {
		if (InBox(c, d, a)) {
			result.Add(a);
		}
	} else if (o4 == 0) {
		if (InBox(c, d, b)) {
			result.Add(b);
		}
	} else {
		const auto t = o3 / (o3 - o4);
		result.Add(Point2 {a.x + t * (b.x - a.x), a.y + t * (b.y - a.y)});
	}
	return result;
}

Point2 ClosestPoint(const Point2 &a, const Point2 &b, const Point2 &p) {
	const auto dx = b.x - a.x;
	const auto dy = b.y - a.y;
	const auto length = dx * dx + dy * dy;
	if (length == 0) {
		return a;
	}
	const auto t = ((p.x - a.x) * dx + (p.y - a.y) * dy) / length;
	if (t <= 0) {
		return a;
	}
	if (t >= 1) {
		return b;
	}
	return Point2 {a.x + t * dx, a.y + t * dy};
}

//----------------------------------------------------------------------------------------------------------------------
// Uniform grids
//----------------------------------------------------------------------------------------------------------------------
uint64_t CellKey(int64_t x, int64_t y) {
	return (static_cast<uint64_t>(x) * 0x9E3779B97F4A7C15ULL) ^ (static_cast<uint64_t>(y) + 0x7F4A7C15ULL);
}

struct Segment {
	uint32_t line;
	uint32_t index;
	Point2 a;
	Point2 b;
};

class SegmentGrid {
public:
	SegmentGrid(const vector<Segment> &segments_p, double tolerance) : segments(segments_p), pad(tolerance) {
		double total = 0;
		for (auto &segment : segments) {
			total += MaxValue(std::fabs(segment.b.x - segment.a.x), std::fabs(segment.b.y - segment.a.y));
		}
		const auto mean = segments.empty() ? 0 : total / static_cast<double>(segments.size());
		cell = MaxValue(mean * 2, tolerance * 2);
		if (!(cell > 0)) {
			cell = 1;
		}
		for (idx_t i = 0; i < segments.size(); i++) {
			Insert(UnsafeNumericCast<uint32_t>(i));
		}
	}

	//! Calls the callback for every pair of segments that share a cell. A pair can be reported more than once.
	template <class CALLBACK>
	void ForEachPair(ClientContext &context, CALLBACK &&callback) const {
		idx_t visited = 0;
		for (auto &entry : cells) {
			auto &list = entry.second;
			for (idx_t i = 0; i < list.size(); i++) {
				for (idx_t j = i + 1; j < list.size(); j++) {
					callback(segments[list[i]], segments[list[j]]);
				}
				if (((++visited) & 0xFFF) == 0) {
					CheckInterrupt(context);
				}
			}
		}
	}

	//! Calls the callback for every segment that may be within the grid tolerance of the point
	template <class CALLBACK>
	void ForEachNear(const Point2 &point, CALLBACK &&callback) const {
		const auto x_min = Cell(point.x - pad);
		const auto x_max = Cell(point.x + pad);
		const auto y_min = Cell(point.y - pad);
		const auto y_max = Cell(point.y + pad);
		for (auto x = x_min; x <= x_max; x++) {
			for (auto y = y_min; y <= y_max; y++) {
				auto entry = cells.find(CellKey(x, y));
				if (entry == cells.end()) {
					continue;
				}
				for (auto index : entry->second) {
					callback(segments[index]);
				}
			}
		}
	}

private:
	int64_t Cell(double value) const {
		return static_cast<int64_t>(std::floor(value / cell));
	}

	void Insert(uint32_t index) {
		auto &segment = segments[index];
		const auto left = segment.a.x <= segment.b.x ? segment.a : segment.b;
		const auto right = segment.a.x <= segment.b.x ? segment.b : segment.a;
		const auto x_min = Cell(left.x - pad);
		const auto x_max = Cell(right.x + pad);
		const auto dx = right.x - left.x;
		for (auto x = x_min; x <= x_max; x++) {
			double y_low = MinValue(left.y, right.y);
			double y_high = MaxValue(left.y, right.y);
			if (dx > 0) {
				// Part of the segment that lies within the padded column
				const auto from = MaxValue(left.x, static_cast<double>(x) * cell - pad);
				const auto to = MinValue(right.x, static_cast<double>(x + 1) * cell + pad);
				const auto y_from = left.y + (from - left.x) / dx * (right.y - left.y);
				const auto y_to = left.y + (to - left.x) / dx * (right.y - left.y);
				y_low = MaxValue(y_low, MinValue(y_from, y_to));
				y_high = MinValue(y_high, MaxValue(y_from, y_to));
			}
			const auto y_min = Cell(y_low - pad) - 1;
			const auto y_max = Cell(y_high + pad) + 1;
			for (auto y = y_min; y <= y_max; y++) {
				cells[CellKey(x, y)].push_back(index);
			}
		}
	}

	const vector<Segment> &segments;
	double pad;
	double cell = 1;
	std::unordered_map<uint64_t, vector<uint32_t>> cells;
};

vector<Segment> CollectSegments(const vector<Line> &lines) {
	vector<Segment> segments;
	for (idx_t i = 0; i < lines.size(); i++) {
		auto &points = lines[i].points;
		for (idx_t j = 0; j + 1 < points.size(); j++) {
			segments.push_back(
			    Segment {UnsafeNumericCast<uint32_t>(i), UnsafeNumericCast<uint32_t>(j), points[j], points[j + 1]});
		}
	}
	if (lines.size() >= NumericLimits<uint32_t>::Maximum() || segments.size() >= NumericLimits<uint32_t>::Maximum()) {
		throw InvalidInputException("Too many geometries");
	}
	return segments;
}

double GetTolerance(RoutingBinder &binder, idx_t argument) {
	const auto tolerance = binder.DoubleArgument(argument, "tolerance");
	if (!(tolerance >= 0) || !std::isfinite(tolerance)) {
		throw BinderException("%s: tolerance must be a non-negative number", binder.name);
	}
	return tolerance;
}

//----------------------------------------------------------------------------------------------------------------------
// pgr_extractVertices
//----------------------------------------------------------------------------------------------------------------------
struct ExtractVerticesData : public RoutingBindData {
	enum class Mode : uint8_t { GEOMETRY, POINTS, IDENTIFIERS };
	Mode mode = Mode::GEOMETRY;

	struct VertexEdges {
		vector<int64_t> in_edges;
		vector<int64_t> out_edges;
	};

	static Value EdgeList(vector<int64_t> &edges, bool has_id) {
		if (!has_id || edges.empty()) {
			return Value(LogicalType::LIST(LogicalType::BIGINT));
		}
		std::sort(edges.begin(), edges.end());
		vector<Value> values;
		for (auto edge : edges) {
			values.push_back(Value::BIGINT(edge));
		}
		return Value::LIST(LogicalType::BIGINT, std::move(values));
	}

	void Compute(ClientContext &context, const RoutingInput &input, RoutingResult &result) const override {
		const auto has_id = input.Has(0);
		if (mode == Mode::IDENTIFIERS) {
			std::map<int64_t, VertexEdges> vertices;
			for (idx_t i = 0; i < input.row_count; i++) {
				const auto id = has_id ? input.Get(0).integers[i] : 0;
				vertices[input.Get(1).integers[i]].out_edges.push_back(id);
				vertices[input.Get(2).integers[i]].in_edges.push_back(id);
			}
			for (auto &entry : vertices) {
				result.BeginRow();
				result.SetInteger(0, entry.first);
				result.SetValue(1, EdgeList(entry.second.in_edges, has_id));
				result.SetValue(2, EdgeList(entry.second.out_edges, has_id));
				result.SetNull(3);
				result.SetNull(4);
				result.SetNull(5);
			}
			return;
		}

		std::map<Point2, VertexEdges> vertices;
		vector<Point2> points;
		for (idx_t i = 0; i < input.row_count; i++) {
			const auto id = has_id ? input.Get(0).integers[i] : 0;
			Point2 start;
			Point2 end;
			if (mode == Mode::GEOMETRY) {
				ReadLinePoints(function_name, input.Get(3), i, points);
				if (points.empty()) {
					continue;
				}
				start = points.front();
				end = points.back();
			} else if (!ReadPoint(function_name, input.Get(4), i, start) ||
			           !ReadPoint(function_name, input.Get(5), i, end)) {
				continue;
			}
			vertices[start].out_edges.push_back(id);
			vertices[end].in_edges.push_back(id);
		}
		int64_t id = 0;
		for (auto &entry : vertices) {
			result.BeginRow();
			result.SetInteger(0, ++id);
			result.SetValue(1, EdgeList(entry.second.in_edges, has_id));
			result.SetValue(2, EdgeList(entry.second.out_edges, has_id));
			result.SetDouble(3, entry.first.x);
			result.SetDouble(4, entry.first.y);
			result.SetString(5, WritePoint(entry.first));
		}
	}
};

unique_ptr<FunctionData> BindExtractVertices(ClientContext &context, TableFunctionBindInput &input,
                                             vector<LogicalType> &return_types, vector<string> &names) {
	auto result = make_uniq<ExtractVerticesData>();
	RoutingBinder binder("pgr_extractVertices", input, 1);
	binder.BindColumns(*result,
	                   {{"id", ColumnKind::INTEGER, false},
	                    {"source", ColumnKind::INTEGER, false},
	                    {"target", ColumnKind::INTEGER, false},
	                    {"geom", ColumnKind::GEOMETRY, false},
	                    {"startpoint", ColumnKind::GEOMETRY, false},
	                    {"endpoint", ColumnKind::GEOMETRY, false}},
	                   "edges");
	binder.Finish();
	auto &columns = result->columns;
	auto geometry_type = LogicalType::GEOMETRY();
	if (columns[3].present) {
		result->mode = ExtractVerticesData::Mode::GEOMETRY;
		geometry_type = columns[3].input_type;
		columns[1].present = columns[2].present = columns[4].present = columns[5].present = false;
	} else if (columns[4].present || columns[5].present) {
		if (!columns[4].present || !columns[5].present) {
			throw BinderException("pgr_extractVertices: the edges input is missing the column \"%s\"",
			                      columns[4].present ? "endpoint" : "startpoint");
		}
		result->mode = ExtractVerticesData::Mode::POINTS;
		geometry_type = columns[4].input_type;
		columns[1].present = columns[2].present = false;
	} else if (columns[1].present || columns[2].present) {
		if (!columns[1].present || !columns[2].present) {
			throw BinderException("pgr_extractVertices: the edges input is missing the column \"%s\"",
			                      columns[1].present ? "target" : "source");
		}
		result->mode = ExtractVerticesData::Mode::IDENTIFIERS;
	} else {
		throw BinderException("pgr_extractVertices: the edges input needs a \"geom\" column, or the \"startpoint\" "
		                      "and \"endpoint\" columns, or the \"source\" and \"target\" columns");
	}
	for (idx_t i = 3; i < 6; i++) {
		binder.AllowNulls(*result, i);
	}
	SetResultSchema(*result, return_types, names,
	                {{"id", LogicalType::BIGINT},
	                 {"in_edges", LogicalType::LIST(LogicalType::BIGINT)},
	                 {"out_edges", LogicalType::LIST(LogicalType::BIGINT)},
	                 {"x", LogicalType::DOUBLE},
	                 {"y", LogicalType::DOUBLE},
	                 {"geom", geometry_type}});
	return std::move(result);
}

//----------------------------------------------------------------------------------------------------------------------
// pgr_createTopology
//----------------------------------------------------------------------------------------------------------------------
class VertexSnapper {
public:
	explicit VertexSnapper(double tolerance_p) : tolerance(tolerance_p) {
	}

	//! Identifier of the nearest vertex within the tolerance, a new vertex when there is none
	int64_t Snap(const Point2 &point) {
		if (tolerance == 0) {
			auto entry = exact.find(point);
			if (entry != exact.end()) {
				return entry->second;
			}
			const auto id = NumericCast<int64_t>(exact.size() + 1);
			exact[point] = id;
			return id;
		}
		const auto cell_x = Cell(point.x);
		const auto cell_y = Cell(point.y);
		idx_t best = vertices.size();
		double best_distance = 0;
		for (auto x = cell_x - 1; x <= cell_x + 1; x++) {
			for (auto y = cell_y - 1; y <= cell_y + 1; y++) {
				auto entry = cells.find(CellKey(x, y));
				if (entry == cells.end()) {
					continue;
				}
				for (auto index : entry->second) {
					const auto distance = Distance(vertices[index], point);
					if (distance <= tolerance &&
					    (best == vertices.size() || distance < best_distance ||
					     (distance == best_distance && index < best))) {
						best = index;
						best_distance = distance;
					}
				}
			}
		}
		if (best == vertices.size()) {
			vertices.push_back(point);
			cells[CellKey(cell_x, cell_y)].push_back(best);
		}
		return NumericCast<int64_t>(best + 1);
	}

private:
	int64_t Cell(double value) const {
		return static_cast<int64_t>(std::floor(value / tolerance));
	}

	double tolerance;
	std::map<Point2, int64_t> exact;
	vector<Point2> vertices;
	std::unordered_map<uint64_t, vector<idx_t>> cells;
};

struct CreateTopologyData : public RoutingBindData {
	double tolerance = 0;
	bool empty = false;

	void Compute(ClientContext &context, const RoutingInput &input, RoutingResult &result) const override {
		if (empty) {
			return;
		}
		const auto lines = ReadLines(function_name, input, 0, 1);
		VertexSnapper snapper(tolerance);
		for (auto &line : lines) {
			result.BeginRow();
			result.SetInteger(0, line.id);
			if (line.points.empty()) {
				result.SetNull(1);
				result.SetNull(2);
				continue;
			}
			result.SetInteger(1, snapper.Snap(line.points.front()));
			result.SetInteger(2, snapper.Snap(line.points.back()));
		}
	}
};

unique_ptr<FunctionData> BindCreateTopology(ClientContext &context, TableFunctionBindInput &input,
                                            vector<LogicalType> &return_types, vector<string> &names) {
	auto result = make_uniq<CreateTopologyData>();
	RoutingBinder binder("pgr_createTopology", input, 2);
	binder.BindColumns(*result, {{"id", ColumnKind::INTEGER, true}, {"geom", ColumnKind::GEOMETRY, true}}, "edges");
	binder.AllowNulls(*result, 1);
	result->tolerance = GetTolerance(binder, 1);
	binder.Finish();
	result->empty = binder.has_null_argument;
	SetResultSchema(*result, return_types, names,
	                {{"id", LogicalType::BIGINT}, {"source", LogicalType::BIGINT}, {"target", LogicalType::BIGINT}});
	return std::move(result);
}

//----------------------------------------------------------------------------------------------------------------------
// pgr_nodeNetwork
//----------------------------------------------------------------------------------------------------------------------
struct NodeNetworkData : public RoutingBindData {
	double tolerance = 0;
	bool empty = false;

	struct Split {
		double measure;
		Point2 point;
	};

	void Compute(ClientContext &context, const RoutingInput &input, RoutingResult &result) const override {
		if (empty) {
			return;
		}
		auto lines = ReadLines(function_name, input, 0, 1);
		lines.erase(std::remove_if(lines.begin(), lines.end(), [](const Line &line) { return !line.HasGeometry(); }),
		            lines.end());
		const auto segments = CollectSegments(lines);

		vector<vector<double>> measures(lines.size());
		for (idx_t i = 0; i < lines.size(); i++) {
			auto &points = lines[i].points;
			measures[i].push_back(0);
			for (idx_t j = 0; j + 1 < points.size(); j++) {
				measures[i].push_back(measures[i].back() + Distance(points[j], points[j + 1]));
			}
		}

		vector<vector<Split>> splits(lines.size());
		auto add_split = [&](const Segment &segment, const Point2 &point) {
			splits[segment.line].push_back(
			    Split {measures[segment.line][segment.index] + Distance(segment.a, point), point});
		};

		SegmentGrid grid(segments, tolerance);
		grid.ForEachPair(context, [&](const Segment &first, const Segment &second) {
			if (first.line == second.line) {
				return;
			}
			const auto intersection = Intersect(first.a, first.b, second.a, second.b);
			for (idx_t i = 0; i < intersection.count; i++) {
				add_split(first, intersection.points[i]);
				add_split(second, intersection.points[i]);
			}
		});

		if (tolerance > 0) {
			// A line that ends within the tolerance of another line splits that line at the nearest location
			for (idx_t i = 0; i < lines.size(); i++) {
				for (auto &end_point : {lines[i].points.front(), lines[i].points.back()}) {
					grid.ForEachNear(end_point, [&](const Segment &segment) {
						if (segment.line == i) {
							return;
						}
						const auto closest = ClosestPoint(segment.a, segment.b, end_point);
						if (Distance(closest, end_point) <= tolerance) {
							add_split(segment, closest);
						}
					});
				}
			}
		}

		int64_t id = 0;
		vector<Point2> piece;
		vector<Split> kept;
		for (idx_t i = 0; i < lines.size(); i++) {
			auto &points = lines[i].points;
			const auto length = measures[i].back();
			auto &line_splits = splits[i];
			std::sort(line_splits.begin(), line_splits.end(), [](const Split &a, const Split &b) {
				if (a.measure != b.measure) {
					return a.measure < b.measure;
				}
				return a.point < b.point;
			});

			// Drop the locations at the ends of the line, and the ones that repeat the previous location
			kept.clear();
			kept.push_back(Split {0, points.front()});
			for (auto &split : line_splits) {
				if (split.measure <= tolerance || split.measure >= length - tolerance) {
					continue;
				}
				if (split.point == kept.back().point || split.measure - kept.back().measure <= tolerance) {
					continue;
				}
				kept.push_back(split);
			}
			kept.push_back(Split {length, points.back()});

			idx_t vertex = 1;
			for (idx_t k = 0; k + 1 < kept.size(); k++) {
				piece.clear();
				piece.push_back(kept[k].point);
				while (vertex + 1 < points.size() && measures[i][vertex] < kept[k + 1].measure) {
					if (measures[i][vertex] > kept[k].measure && points[vertex] != piece.back()) {
						piece.push_back(points[vertex]);
					}
					vertex++;
				}
				if (kept[k + 1].point != piece.back() || piece.size() == 1) {
					piece.push_back(kept[k + 1].point);
				}
				result.BeginRow();
				result.SetInteger(0, ++id);
				result.SetInteger(1, lines[i].id);
				result.SetInteger(2, NumericCast<int64_t>(k + 1));
				result.SetString(3, WriteWKB(2, piece.data(), piece.size()));
			}
		}
	}
};

unique_ptr<FunctionData> BindNodeNetwork(ClientContext &context, TableFunctionBindInput &input,
                                         vector<LogicalType> &return_types, vector<string> &names) {
	auto result = make_uniq<NodeNetworkData>();
	RoutingBinder binder("pgr_nodeNetwork", input, 2);
	binder.BindColumns(*result, {{"id", ColumnKind::INTEGER, true}, {"geom", ColumnKind::GEOMETRY, true}}, "edges");
	binder.AllowNulls(*result, 1);
	result->tolerance = GetTolerance(binder, 1);
	binder.Finish();
	result->empty = binder.has_null_argument;
	SetResultSchema(*result, return_types, names,
	                {{"id", LogicalType::BIGINT},
	                 {"old_id", LogicalType::BIGINT},
	                 {"sub_id", LogicalType::INTEGER},
	                 {"geom", result->columns[1].input_type}});
	return std::move(result);
}

//----------------------------------------------------------------------------------------------------------------------
// pgr_analyzeGraph
//----------------------------------------------------------------------------------------------------------------------
struct AnalyzeGraphData : public RoutingBindData {
	double tolerance = 0;
	bool empty = false;

	static bool IsSimple(const Line &line) {
		auto &points = line.points;
		const auto count = points.size() - 1;
		for (idx_t i = 0; i < count; i++) {
			for (idx_t j = i + 1; j < count; j++) {
				const auto intersection = Intersect(points[i], points[i + 1], points[j], points[j + 1]);
				if (intersection.count == 0) {
					continue;
				}
				const auto adjacent = j == i + 1 || (i == 0 && j + 1 == count);
				if (!adjacent || intersection.overlap) {
					return false;
				}
				const auto &shared = j == i + 1 ? points[j] : points[0];
				if (intersection.points[0] != shared) {
					return false;
				}
			}
		}
		return true;
	}

	void Compute(ClientContext &context, const RoutingInput &input, RoutingResult &result) const override {
		if (empty) {
			return;
		}
		vector<Line> lines(input.row_count);
		for (idx_t i = 0; i < input.row_count; i++) {
			lines[i].id = input.Get(0).integers[i];
			lines[i].source = input.Get(1).integers[i];
			lines[i].target = input.Get(2).integers[i];
			ReadLinePoints(function_name, input.Get(3), i, lines[i].points);
		}

		std::unordered_map<int64_t, idx_t> degree;
		for (auto &line : lines) {
			degree[line.source]++;
			degree[line.target]++;
		}

		int64_t isolated = 0;
		int64_t dead_ends = 0;
		int64_t gaps = 0;
		int64_t rings = 0;
		for (auto &entry : degree) {
			if (entry.second == 1) {
				dead_ends++;
			}
		}

		const auto segments = CollectSegments(lines);
		SegmentGrid grid(segments, tolerance);
		for (auto &line : lines) {
			const auto source_dead = degree[line.source] == 1;
			const auto target_dead = degree[line.target] == 1;
			if (source_dead && target_dead) {
				isolated++;
			}
			if (!line.HasGeometry()) {
				continue;
			}
			if (line.Closed() && line.points.size() >= 4 && IsSimple(line)) {
				rings++;
			}
			for (idx_t side = 0; side < 2; side++) {
				if (!(side == 0 ? source_dead : target_dead)) {
					continue;
				}
				const auto vertex = side == 0 ? line.source : line.target;
				const auto &location = side == 0 ? line.points.front() : line.points.back();
				bool found = false;
				grid.ForEachNear(location, [&](const Segment &segment) {
					auto &other = lines[segment.line];
					if (found || other.source == vertex || other.target == vertex) {
						return;
					}
					if (Distance(ClosestPoint(segment.a, segment.b, location), location) <= tolerance) {
						found = true;
					}
				});
				if (found) {
					gaps++;
				}
			}
		}

		// Pairs of lines that cross: they share a point that is interior to both, and no stretch of line
		static constexpr const uint8_t CROSSING = 1;
		static constexpr const uint8_t OVERLAP = 2;
		std::unordered_map<uint64_t, uint8_t> pairs;
		grid.ForEachPair(context, [&](const Segment &first, const Segment &second) {
			if (first.line == second.line) {
				return;
			}
			const auto intersection = Intersect(first.a, first.b, second.a, second.b);
			if (intersection.count == 0) {
				return;
			}
			const auto low = MinValue(first.line, second.line);
			const auto high = MaxValue(first.line, second.line);
			auto &flags = pairs[(static_cast<uint64_t>(low) << 32) | high];
			if (intersection.overlap) {
				flags |= OVERLAP;
				return;
			}
			auto interior = [&](const Line &line, const Point2 &point) {
				return line.Closed() || (point != line.points.front() && point != line.points.back());
			};
			if (interior(lines[first.line], intersection.points[0]) &&
			    interior(lines[second.line], intersection.points[0])) {
				flags |= CROSSING;
			}
		});
		int64_t intersections = 0;
		for (auto &entry : pairs) {
			if (entry.second == CROSSING) {
				intersections++;
			}
		}

		const vector<std::pair<const char *, int64_t>> rows = {{"isolated_segments", isolated},
		                                                       {"dead_ends", dead_ends},
		                                                       {"potential_gaps", gaps},
		                                                       {"intersections", intersections},
		                                                       {"ring_geometries", rings}};
		for (auto &row : rows) {
			result.BeginRow();
			result.SetString(0, row.first);
			result.SetInteger(1, row.second);
		}
	}
};

unique_ptr<FunctionData> BindAnalyzeGraph(ClientContext &context, TableFunctionBindInput &input,
                                          vector<LogicalType> &return_types, vector<string> &names) {
	auto result = make_uniq<AnalyzeGraphData>();
	RoutingBinder binder("pgr_analyzeGraph", input, 2);
	binder.BindColumns(*result,
	                   {{"id", ColumnKind::INTEGER, true},
	                    {"source", ColumnKind::INTEGER, true},
	                    {"target", ColumnKind::INTEGER, true},
	                    {"geom", ColumnKind::GEOMETRY, true}},
	                   "edges");
	binder.AllowNulls(*result, 3);
	result->tolerance = GetTolerance(binder, 1);
	binder.Finish();
	result->empty = binder.has_null_argument;
	SetResultSchema(*result, return_types, names, {{"metric", LogicalType::VARCHAR}, {"count", LogicalType::BIGINT}});
	return std::move(result);
}

} // namespace

void RegisterTopologyFunctions(ExtensionLoader &loader) {
	RegisterRoutingFunction(loader, "pgr_extractVertices", 0, BindExtractVertices, {},
	                        R"(
Vertices of a graph, extracted from its edges.

`pgr_extractVertices(edges)`

The edges are a table-valued argument, i.e. a parenthesised subquery, whose columns are matched by name. One of the following sets of columns is used, in this order of preference:

- `geom` (LINESTRING): the vertices are the distinct start and end points of the lines. They are numbered from 1 by increasing `x`, then `y`.
- `startpoint` and `endpoint` (POINT): same as above, with the end points given explicitly.
- `source` and `target` (integer): the vertices are the distinct identifiers. `x`, `y` and `geom` are NULL.

`id` (integer) is optional. When it is present `in_edges` and `out_edges` are filled, otherwise they are NULL. Rows whose geometry is NULL or empty are skipped.

| Column | Type | Description |
| --- | --- | --- |
| `id` | BIGINT | Identifier of the vertex |
| `in_edges` | BIGINT[] | Sorted identifiers of the edges that end at the vertex, NULL when there is none |
| `out_edges` | BIGINT[] | Sorted identifiers of the edges that start at the vertex, NULL when there is none |
| `x` | DOUBLE | X coordinate of the vertex |
| `y` | DOUBLE | Y coordinate of the vertex |
| `geom` | GEOMETRY | POINT geometry of the vertex, in the coordinate system of the input |

Differences with pgRouting: the edges are a table-valued argument instead of an SQL string, there is no `dryrun` option, and only the X and Y coordinates are considered.
)",
	                        R"(
CREATE TABLE vertices AS SELECT * FROM pgr_extractVertices((SELECT id, geom FROM edges));

-- fill the source and target of the edges
SELECT e.id, s.id AS source, t.id AS target
FROM edges e
JOIN vertices s ON ST_Equals(ST_StartPoint(e.geom), s.geom)
JOIN vertices t ON ST_Equals(ST_EndPoint(e.geom), t.geom);
)");

	RegisterRoutingFunction(loader, "pgr_createTopology", 1, BindCreateTopology, {},
	                        R"(
Builds the topology of a network: gives each edge the identifier of its start and end vertex, snapping end points that are within a tolerance of each other to the same vertex.

`pgr_createTopology(edges, tolerance)`

The edges are a table-valued argument with the columns `id` (integer) and `geom` (LINESTRING). `tolerance` is a non-negative constant in the unit of the coordinates; with 0 only identical points are merged.

The edges are processed by increasing `id`, the start point before the end point. A point takes the identifier of the nearest existing vertex within the tolerance, otherwise it becomes a new vertex with the next identifier, starting from 1. The result therefore does not depend on the order of the input rows.

| Column | Type | Description |
| --- | --- | --- |
| `id` | BIGINT | Identifier of the edge |
| `source` | BIGINT | Identifier of the vertex at the start of the edge, NULL when the geometry is NULL or empty |
| `target` | BIGINT | Identifier of the vertex at the end of the edge, NULL when the geometry is NULL or empty |

Differences with pgRouting: `pgr_createTopology` updated the `source` and `target` columns of the edge table and created a vertices table. A table function cannot alter its input, so the assignment is returned as rows instead, to be joined back or stored by the caller; the vertices can be obtained with `pgr_extractVertices`. The function was removed from pgRouting 4.0 and is kept here for convenience.
)",
	                        R"(
CREATE TABLE network AS
SELECT e.*, t.source, t.target
FROM edges e JOIN pgr_createTopology((SELECT id, geom FROM edges), 0.001) t USING (id);
)");

	RegisterRoutingFunction(loader, "pgr_nodeNetwork", 1, BindNodeNetwork, {},
	                        R"(
Nodes a network: splits the lines where they meet, so that lines only touch at their end points.

`pgr_nodeNetwork(edges, tolerance)`

The edges are a table-valued argument with the columns `id` (integer) and `geom` (LINESTRING). `tolerance` is a non-negative constant in the unit of the coordinates.

A line is split where another line crosses or touches it, at both ends of a stretch it shares with another line, and where another line ends within the tolerance of it without touching it. No split is made within the tolerance of an end of the line or of the previous split. A line does not split itself. Every line is returned, in one piece when nothing splits it.

| Column | Type | Description |
| --- | --- | --- |
| `id` | BIGINT | Identifier of the new edge, from 1, by `old_id` and `sub_id` |
| `old_id` | BIGINT | Identifier of the original edge |
| `sub_id` | INTEGER | Position of the piece along the original edge, starting from 1 |
| `geom` | GEOMETRY | LINESTRING geometry of the piece, in the coordinate system of the input |

Rows whose geometry is NULL or empty are skipped. Only the X and Y coordinates are kept.

Differences with pgRouting: `pgr_nodeNetwork` created a new table. A table function cannot create tables, so the noded edges are returned as rows. The function was removed from pgRouting 4.0 and is kept here for convenience.
)",
	                        R"(
CREATE TABLE edges_noded AS SELECT * FROM pgr_nodeNetwork((SELECT id, geom FROM edges), 0.001);
)");

	RegisterRoutingFunction(loader, "pgr_analyzeGraph", 1, BindAnalyzeGraph, {},
	                        R"(
Summary of the usual problems of a network topology.

`pgr_analyzeGraph(edges, tolerance)`

The edges are a table-valued argument with the columns `id`, `source`, `target` (integers) and `geom` (LINESTRING). `tolerance` is a non-negative constant in the unit of the coordinates.

The result has the columns `metric` (VARCHAR) and `count` (BIGINT), with one row per metric, in this order:

| Metric | Description |
| --- | --- |
| `isolated_segments` | Edges whose two end vertices are not used by any other edge |
| `dead_ends` | Vertices that are used by a single edge end |
| `potential_gaps` | Dead ends that are within the tolerance of an edge that is not connected to them: probably a missing connection |
| `intersections` | Pairs of edges that cross each other away from their end points: probably a missing vertex |
| `ring_geometries` | Edges whose geometry is a closed and simple line |

Edges with a NULL or empty geometry count for the first two metrics only.

Differences with pgRouting: `pgr_analyzeGraph` reported the counts as notices and stored flags in the vertices table. A table function cannot alter tables, so the counts are returned as rows and the vertices are derived from the `source` and `target` columns of the edges. The function was removed from pgRouting 4.0 and is kept here for convenience.
)",
	                        R"(
SELECT * FROM pgr_analyzeGraph((SELECT id, source, target, geom FROM edges), 0.001);
)");
}

} // namespace routing

} // namespace duckdb
