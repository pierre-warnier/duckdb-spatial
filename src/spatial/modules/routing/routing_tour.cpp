#include "spatial/modules/routing/routing_module.hpp"
#include "spatial/modules/routing/routing_graph.hpp"

#include "duckdb/common/exception.hpp"

#include <algorithm>

namespace duckdb {

namespace routing {

namespace {

//----------------------------------------------------------------------------------------------------------------------
// pgr_TSP
//----------------------------------------------------------------------------------------------------------------------
class TourSolver {
public:
	static constexpr const idx_t EXACT_LIMIT = 12;

	TourSolver(ClientContext &context_p, const vector<double> &matrix_p, idx_t size_p, bool fixed_end_p)
	    : context(context_p), matrix(matrix_p), size(size_p), fixed_end(fixed_end_p) {
	}

	//! Node 0 is the start. When the end is fixed it is node `size - 1` and stays the last stop before the return.
	vector<uint32_t> Solve() {
		if (size <= 3) {
			vector<uint32_t> order;
			for (uint32_t i = 0; i < size; i++) {
				order.push_back(i);
			}
			return order;
		}
		if (size <= EXACT_LIMIT) {
			return SolveExact();
		}
		auto order = NearestNeighbour();
		Improve(order);
		return order;
	}

private:
	double Cost(uint32_t from, uint32_t to) const {
		return matrix[from * size + to];
	}

	vector<uint32_t> SolveExact() {
		// Held-Karp over the nodes other than the start
		const auto count = size - 1;
		const auto full = (idx_t(1) << count) - 1;
		vector<double> best((full + 1) * count, Infinity());
		vector<uint8_t> parent((full + 1) * count, 0xFF);
		for (idx_t j = 0; j < count; j++) {
			best[(idx_t(1) << j) * count + j] = Cost(0, UnsafeNumericCast<uint32_t>(j + 1));
		}
		for (idx_t mask = 1; mask <= full; mask++) {
			for (idx_t j = 0; j < count; j++) {
				const auto current = best[mask * count + j];
				if (!(mask & (idx_t(1) << j)) || current == Infinity()) {
					continue;
				}
				for (idx_t k = 0; k < count; k++) {
					if (mask & (idx_t(1) << k)) {
						continue;
					}
					const auto next_mask = mask | (idx_t(1) << k);
					const auto candidate =
					    current + Cost(UnsafeNumericCast<uint32_t>(j + 1), UnsafeNumericCast<uint32_t>(k + 1));
					if (candidate < best[next_mask * count + k]) {
						best[next_mask * count + k] = candidate;
						parent[next_mask * count + k] = UnsafeNumericCast<uint8_t>(j);
					}
				}
			}
		}
		idx_t last = count - 1;
		if (!fixed_end) {
			double best_total = Infinity();
			for (idx_t j = 0; j < count; j++) {
				const auto total = best[full * count + j] + Cost(UnsafeNumericCast<uint32_t>(j + 1), 0);
				if (total < best_total) {
					best_total = total;
					last = j;
				}
			}
		}
		vector<uint32_t> order;
		auto mask = full;
		auto current = last;
		while (true) {
			order.push_back(UnsafeNumericCast<uint32_t>(current + 1));
			const auto previous = parent[mask * count + current];
			mask &= ~(idx_t(1) << current);
			if (previous == 0xFF) {
				break;
			}
			current = previous;
		}
		order.push_back(0);
		std::reverse(order.begin(), order.end());
		return order;
	}

	vector<uint32_t> NearestNeighbour() {
		vector<uint8_t> visited(size, 0);
		vector<uint32_t> order;
		order.push_back(0);
		visited[0] = 1;
		const auto free_count = fixed_end ? size - 1 : size;
		while (order.size() < free_count) {
			const auto current = order.back();
			uint32_t next = INVALID_VERTEX;
			for (uint32_t candidate = 1; candidate < free_count; candidate++) {
				if (!visited[candidate] && (next == INVALID_VERTEX || Cost(current, candidate) < Cost(current, next))) {
					next = candidate;
				}
			}
			visited[next] = 1;
			order.push_back(next);
		}
		if (fixed_end) {
			order.push_back(UnsafeNumericCast<uint32_t>(size - 1));
		}
		return order;
	}

	void Improve(vector<uint32_t> &order) {
		const auto movable_end = fixed_end ? size - 1 : size;
		const double epsilon = 1e-12;
		bool improved = true;
		while (improved) {
			CheckInterrupt(context);
			improved = false;
			// 2-opt: reverse order[i..j]
			for (idx_t i = 1; i + 1 < movable_end; i++) {
				for (idx_t j = i + 1; j < movable_end; j++) {
					const auto before = order[i - 1];
					const auto after = order[(j + 1) % size];
					const auto delta = Cost(before, order[j]) + Cost(order[i], after) - Cost(before, order[i]) -
					                   Cost(order[j], after);
					if (delta < -epsilon) {
						std::reverse(order.begin() + NumericCast<int64_t>(i), order.begin() + NumericCast<int64_t>(j + 1));
						improved = true;
					}
				}
			}
			// Or-opt: move a chain of up to three stops elsewhere
			for (idx_t length = 1; length <= 3; length++) {
				for (idx_t i = 1; i + length <= movable_end; i++) {
					const auto before = order[i - 1];
					const auto first = order[i];
					const auto last = order[i + length - 1];
					const auto after = order[(i + length) % size];
					const auto removal = Cost(before, after) - Cost(before, first) - Cost(last, after);
					for (idx_t position = 0; position < movable_end; position++) {
						if (position + 1 >= i && position < i + length) {
							continue;
						}
						const auto left = order[position];
						const auto right = order[(position + 1) % size];
						const auto forward = Cost(left, first) + Cost(last, right) - Cost(left, right);
						const auto backward = Cost(left, last) + Cost(first, right) - Cost(left, right);
						const auto reversed = backward < forward;
						if (removal + MinValue(forward, backward) < -epsilon) {
							vector<uint32_t> chain(order.begin() + NumericCast<int64_t>(i),
							                       order.begin() + NumericCast<int64_t>(i + length));
							if (reversed) {
								std::reverse(chain.begin(), chain.end());
							}
							order.erase(order.begin() + NumericCast<int64_t>(i),
							            order.begin() + NumericCast<int64_t>(i + length));
							const auto insert_at = position < i ? position + 1 : position + 1 - length;
							order.insert(order.begin() + NumericCast<int64_t>(insert_at), chain.begin(), chain.end());
							improved = true;
							break;
						}
					}
				}
			}
		}
	}

	ClientContext &context;
	const vector<double> &matrix;
	idx_t size;
	bool fixed_end;
};

struct TourData : public RoutingBindData {
	int64_t start_id = 0;
	int64_t end_id = 0;
	bool empty = false;

	static idx_t Find(const vector<int64_t> &ids, int64_t id) {
		auto entry = std::lower_bound(ids.begin(), ids.end(), id);
		if (entry == ids.end() || *entry != id) {
			return ids.size();
		}
		return NumericCast<idx_t>(entry - ids.begin());
	}

	void Compute(ClientContext &context, const RoutingInput &input, RoutingResult &result) const override {
		if (empty) {
			return;
		}
		auto &from = input.Get(0).integers;
		auto &to = input.Get(1).integers;
		auto &costs = input.Get(2).numerics;

		vector<int64_t> ids;
		for (idx_t i = 0; i < input.row_count; i++) {
			if (costs[i] >= 0 && from[i] != to[i]) {
				ids.push_back(from[i]);
				ids.push_back(to[i]);
			}
		}
		std::sort(ids.begin(), ids.end());
		ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
		const auto count = ids.size();
		if (count == 0) {
			return;
		}

		auto start = start_id;
		auto end = end_id;
		if (start == 0) {
			std::swap(start, end);
		}
		if (start != 0 && Find(ids, start) == count) {
			throw InvalidInputException("pgr_TSP: start_id %lld does not exist in the matrix", start);
		}
		if (end != 0 && Find(ids, end) == count) {
			throw InvalidInputException("pgr_TSP: end_id %lld does not exist in the matrix", end);
		}
		if (end == start) {
			end = 0;
		}

		// Symmetric matrix: the cheapest of both directions, missing cells completed with shortest paths
		vector<double> full(count * count, Infinity());
		for (idx_t i = 0; i < count; i++) {
			full[i * count + i] = 0;
		}
		for (idx_t i = 0; i < input.row_count; i++) {
			if (!(costs[i] >= 0) || from[i] == to[i]) {
				continue;
			}
			const auto a = Find(ids, from[i]);
			const auto b = Find(ids, to[i]);
			if (costs[i] < full[a * count + b]) {
				full[a * count + b] = costs[i];
				full[b * count + a] = costs[i];
			}
		}
		bool complete = true;
		for (auto value : full) {
			if (value == Infinity()) {
				complete = false;
				break;
			}
		}
		if (!complete) {
			for (idx_t k = 0; k < count; k++) {
				CheckInterrupt(context);
				for (idx_t i = 0; i < count; i++) {
					const auto first = full[i * count + k];
					if (first == Infinity()) {
						continue;
					}
					for (idx_t j = 0; j < count; j++) {
						const auto candidate = first + full[k * count + j];
						if (candidate < full[i * count + j]) {
							full[i * count + j] = candidate;
						}
					}
				}
			}
		}

		// The tour covers the connected component of the start node
		const auto start_idx = start != 0 ? Find(ids, start) : 0;
		const auto end_idx = end != 0 ? Find(ids, end) : count;
		if (end_idx != count && full[start_idx * count + end_idx] == Infinity()) {
			throw InvalidInputException("pgr_TSP: start_id %lld and end_id %lld are not connected", start, end);
		}
		vector<idx_t> nodes;
		nodes.push_back(start_idx);
		for (idx_t i = 0; i < count; i++) {
			if (i != start_idx && i != end_idx && full[start_idx * count + i] != Infinity()) {
				nodes.push_back(i);
			}
		}
		if (end_idx != count) {
			nodes.push_back(end_idx);
		}
		const auto size = nodes.size();
		vector<double> matrix(size * size);
		for (idx_t i = 0; i < size; i++) {
			for (idx_t j = 0; j < size; j++) {
				matrix[i * size + j] = full[nodes[i] * count + nodes[j]];
			}
		}

		TourSolver solver(context, matrix, size, end_idx != count);
		const auto order = solver.Solve();

		double agg_cost = 0;
		for (idx_t i = 0; i <= size; i++) {
			const auto node = order[i % size];
			const auto cost = i == 0 ? 0 : matrix[order[i - 1] * size + node];
			agg_cost += cost;
			result.BeginRow();
			result.SetInteger(0, NumericCast<int64_t>(i + 1));
			result.SetInteger(1, ids[nodes[node]]);
			result.SetDouble(2, cost);
			result.SetDouble(3, agg_cost);
		}
	}
};

unique_ptr<FunctionData> BindTour(ClientContext &context, TableFunctionBindInput &input,
                                  vector<LogicalType> &return_types, vector<string> &names) {
	auto result = make_uniq<TourData>();
	RoutingBinder binder("pgr_TSP", input, 1);
	binder.BindColumns(*result,
	                   {{"start_vid", ColumnKind::INTEGER, true},
	                    {"end_vid", ColumnKind::INTEGER, true},
	                    {"agg_cost", ColumnKind::NUMERIC, true}},
	                   "matrix");
	result->start_id = binder.Option("start_id", LogicalType::BIGINT, Value::BIGINT(0)).GetValue<int64_t>();
	result->end_id = binder.Option("end_id", LogicalType::BIGINT, Value::BIGINT(0)).GetValue<int64_t>();
	binder.Finish();
	result->empty = binder.has_null_argument;
	SetResultSchema(*result, return_types, names,
	                {{"seq", LogicalType::INTEGER},
	                 {"node", LogicalType::BIGINT},
	                 {"cost", LogicalType::DOUBLE},
	                 {"agg_cost", LogicalType::DOUBLE}});
	return std::move(result);
}

} // namespace

void RegisterTourFunctions(ExtensionLoader &loader) {
	RegisterRoutingFunction(loader, "pgr_TSP", 0, BindTour,
	                        {{"start_id", LogicalType::BIGINT}, {"end_id", LogicalType::BIGINT}},
	                        R"(
Travelling salesperson tour over a cost matrix: a round trip that visits every node once.

`pgr_TSP(matrix, [start_id := 0, end_id := 0])`

The matrix is a table-valued argument, typically the result of `pgr_dijkstraCostMatrix` with `directed := false`, whose columns are matched by name:

| Column | Type | Description |
| --- | --- | --- |
| `start_vid` | integer | Identifier of the starting node |
| `end_vid` | integer | Identifier of the ending node |
| `agg_cost` | numeric | Cost to go from `start_vid` to `end_vid` |

The problem is solved on an undirected graph: when the costs of the two directions differ the smallest one is used, rows with a negative cost and rows from a node to itself are ignored, and missing cells are completed with the cost of the shortest path through the other nodes. Nodes that cannot be reached from the start node are left out of the tour.

`start_id` is the node where the tour starts and ends, by default (0) the smallest node identifier. When `end_id` is given and differs from `start_id`, it is the last node visited before returning to the start.

The result is ordered by `seq`:

| Column | Type | Description |
| --- | --- | --- |
| `seq` | INTEGER | Sequential value starting from 1 |
| `node` | BIGINT | Identifier of the node at this position. The start node is repeated in the last row |
| `cost` | DOUBLE | Cost to travel from the previous node to `node`, 0 for the first row |
| `agg_cost` | DOUBLE | Aggregate cost from the start node to `node` |

The tour is optimal up to 12 nodes (exact dynamic programming). Beyond that it is a heuristic: a nearest neighbour tour improved with 2-opt and Or-opt moves until no move shortens it, which gives a good but not necessarily optimal tour.

Differences with pgRouting: the matrix is a table-valued argument instead of an SQL string; pgRouting uses the metric approximation of the Boost graph library, so the tours differ although both are valid (the tour returned here is never longer than the nearest neighbour tour, and does not depend on the order of the input rows); and a start and end node that are not connected raise an error instead of being joined with an estimated cost.
)",
	                        R"(
SELECT * FROM pgr_TSP((
    SELECT * FROM pgr_dijkstraCostMatrix((SELECT id, source, target, cost, reverse_cost FROM edges), [1, 5, 9, 15], directed := false)
), start_id := 1);
)");
}

} // namespace routing

} // namespace duckdb
