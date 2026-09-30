#include "spatial/modules/routing/routing_module.hpp"
#include "spatial/modules/routing/routing_search.hpp"

#include <algorithm>

namespace duckdb {

namespace routing {

namespace {

//----------------------------------------------------------------------------------------------------------------------
// pgr_connectedComponents, pgr_strongComponents
//----------------------------------------------------------------------------------------------------------------------
struct ComponentData : public RoutingBindData {
	bool strong = false;

	static vector<uint32_t> Connected(const Graph &graph) {
		const auto count = graph.VertexCount();
		vector<uint32_t> component(count, INVALID_VERTEX);
		vector<uint32_t> stack;
		for (uint32_t root = 0; root < count; root++) {
			if (component[root] != INVALID_VERTEX) {
				continue;
			}
			component[root] = root;
			stack.push_back(root);
			while (!stack.empty()) {
				const auto vertex = stack.back();
				stack.pop_back();
				for (auto i = graph.out_offsets[vertex]; i < graph.out_offsets[vertex + 1]; i++) {
					const auto head = graph.out_arcs[i].head;
					if (component[head] == INVALID_VERTEX) {
						component[head] = root;
						stack.push_back(head);
					}
				}
			}
		}
		return component;
	}

	//! Tarjan's algorithm, without recursion
	static vector<uint32_t> Strong(const Graph &graph) {
		const auto count = graph.VertexCount();
		vector<uint32_t> index(count, INVALID_VERTEX);
		vector<uint32_t> low(count, 0);
		vector<uint32_t> component(count, INVALID_VERTEX);
		vector<uint8_t> on_stack(count, 0);
		vector<uint32_t> stack;
		vector<uint32_t> cursor(graph.out_offsets.begin(), graph.out_offsets.end() - 1);
		vector<uint32_t> call_stack;
		uint32_t next_index = 0;

		for (uint32_t root = 0; root < count; root++) {
			if (index[root] != INVALID_VERTEX) {
				continue;
			}
			call_stack.push_back(root);
			while (!call_stack.empty()) {
				const auto vertex = call_stack.back();
				if (index[vertex] == INVALID_VERTEX) {
					index[vertex] = low[vertex] = next_index++;
					stack.push_back(vertex);
					on_stack[vertex] = 1;
				}
				bool descended = false;
				while (cursor[vertex] < graph.out_offsets[vertex + 1]) {
					const auto head = graph.out_arcs[cursor[vertex]++].head;
					if (index[head] == INVALID_VERTEX) {
						call_stack.push_back(head);
						descended = true;
						break;
					}
					if (on_stack[head]) {
						low[vertex] = MinValue(low[vertex], index[head]);
					}
				}
				if (descended) {
					continue;
				}
				call_stack.pop_back();
				if (!call_stack.empty()) {
					const auto parent = call_stack.back();
					low[parent] = MinValue(low[parent], low[vertex]);
				}
				if (low[vertex] == index[vertex]) {
					auto first = stack.size();
					uint32_t smallest = vertex;
					while (true) {
						first--;
						smallest = MinValue(smallest, stack[first]);
						if (stack[first] == vertex) {
							break;
						}
					}
					for (auto i = first; i < stack.size(); i++) {
						component[stack[i]] = smallest;
						on_stack[stack[i]] = 0;
					}
					stack.resize(first);
				}
			}
		}
		return component;
	}

	void Compute(ClientContext &context, const RoutingInput &input, RoutingResult &result) const override {
		Graph graph(ReadEdges(input, EDGE_COLUMN_INDEXES), strong);
		const auto component = strong ? Strong(graph) : Connected(graph);

		vector<std::pair<uint32_t, uint32_t>> rows;
		rows.reserve(component.size());
		for (uint32_t vertex = 0; vertex < graph.VertexCount(); vertex++) {
			rows.emplace_back(component[vertex], vertex);
		}
		std::sort(rows.begin(), rows.end());
		int64_t seq = 0;
		for (auto &row : rows) {
			result.BeginRow();
			result.SetInteger(0, ++seq);
			result.SetInteger(1, graph.vertex_ids[row.first]);
			result.SetInteger(2, graph.vertex_ids[row.second]);
		}
	}
};

template <bool STRONG>
unique_ptr<FunctionData> BindComponents(ClientContext &context, TableFunctionBindInput &input,
                                        vector<LogicalType> &return_types, vector<string> &names) {
	auto result = make_uniq<ComponentData>();
	result->strong = STRONG;
	RoutingBinder binder(input.table_function.name.c_str(), input, 1);
	binder.BindColumns(*result, EDGE_COLUMN_SPECS, "edges");
	binder.Finish();
	SetResultSchema(*result, return_types, names,
	                {{"seq", LogicalType::BIGINT}, {"component", LogicalType::BIGINT}, {"node", LogicalType::BIGINT}});
	return std::move(result);
}

const char *const COMPONENT_DOC = R"(
The edges are a table-valued argument with the columns `id`, `source`, `target`, `cost` and optionally `reverse_cost`, as for `pgr_dijkstra`. A negative `cost` or `reverse_cost` means the edge does not exist in that direction; its end points are still vertices of the graph.

The result is ordered by `component` and `node`:

| Column | Type | Description |
| --- | --- | --- |
| `seq` | BIGINT | Sequential value starting from 1 |
| `component` | BIGINT | Identifier of the component: the smallest vertex identifier it contains |
| `node` | BIGINT | Identifier of a vertex of the component |

Differences with pgRouting: the edges are a table-valued argument instead of an SQL string.
)";

} // namespace

void RegisterComponentFunctions(ExtensionLoader &loader) {
	RegisterRoutingFunction(loader, "pgr_connectedComponents", 0, BindComponents<false>, {},
	                        (string(R"(
Connected components of an undirected graph: two vertices are in the same component when a path exists between them, whatever the direction of the edges.

`pgr_connectedComponents(edges)`
)") + COMPONENT_DOC)
	                            .c_str(),
	                        R"(
SELECT * FROM pgr_connectedComponents((SELECT id, source, target, cost, reverse_cost FROM edges));
)");

	RegisterRoutingFunction(loader, "pgr_strongComponents", 0, BindComponents<true>, {},
	                        (string(R"(
Strongly connected components of a directed graph, using Tarjan's algorithm: two vertices are in the same component when each one can be reached from the other.

`pgr_strongComponents(edges)`
)") + COMPONENT_DOC)
	                            .c_str(),
	                        R"(
SELECT * FROM pgr_strongComponents((SELECT id, source, target, cost, reverse_cost FROM edges));
)");
}

} // namespace routing

} // namespace duckdb
