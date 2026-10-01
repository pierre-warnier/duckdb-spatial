#pragma once

#include "duckdb.hpp"
#include "duckdb/function/table_function.hpp"
#include "duckdb/common/insertion_order_preserving_map.hpp"

namespace duckdb {

class ExtensionLoader;

namespace routing {

//----------------------------------------------------------------------------------------------------------------------
// Input columns
//----------------------------------------------------------------------------------------------------------------------
enum class ColumnKind : uint8_t { INTEGER, NUMERIC, GEOMETRY };

struct ColumnSpec {
	const char *name;
	ColumnKind kind;
	bool required;
};

struct InputColumn {
	string name;
	ColumnKind kind = ColumnKind::INTEGER;
	bool present = false;
	bool nullable = false;
	idx_t input_index = 0;
	LogicalType input_type;

	vector<int64_t> integers;
	vector<double> numerics;
	vector<string> blobs;
	vector<bool> validity;
};

class RoutingInput {
public:
	idx_t row_count = 0;
	vector<InputColumn> columns;

	const InputColumn &Get(idx_t idx) const {
		return columns[idx];
	}
	bool Has(idx_t idx) const {
		return columns[idx].present;
	}
};

//----------------------------------------------------------------------------------------------------------------------
// Result buffer
//----------------------------------------------------------------------------------------------------------------------
class RoutingResult {
public:
	void Initialize(const vector<LogicalType> &types);

	idx_t RowCount() const {
		return row_count;
	}
	idx_t ColumnCount() const {
		return columns.size();
	}

	void BeginRow();
	void SetInteger(idx_t col, int64_t value);
	void SetDouble(idx_t col, double value);
	void SetValue(idx_t col, Value value);
	void SetString(idx_t col, string value);
	void SetNull(idx_t col);

	void Emit(idx_t &offset, DataChunk &output) const;

private:
	enum class Storage : uint8_t { INTEGER, DOUBLE, STRING, VALUE };
	struct Column {
		LogicalType type;
		Storage storage;
		vector<int64_t> integers;
		vector<double> doubles;
		vector<string> strings;
		vector<Value> values;
		vector<bool> validity;
	};
	vector<Column> columns;
	idx_t row_count = 0;
};

//----------------------------------------------------------------------------------------------------------------------
// Bind data
//----------------------------------------------------------------------------------------------------------------------
struct RoutingBindData : public TableFunctionData {
	string function_name;
	vector<InputColumn> columns;
	vector<LogicalType> result_types;

	virtual void Compute(ClientContext &context, const RoutingInput &input, RoutingResult &result) const = 0;
};

//----------------------------------------------------------------------------------------------------------------------
// Bind helpers
//----------------------------------------------------------------------------------------------------------------------
struct RoutingBinder {
	RoutingBinder(const char *name, TableFunctionBindInput &input, idx_t required_arguments);

	const char *name;
	TableFunctionBindInput &input;
	idx_t required_arguments;
	idx_t next_positional;
	bool has_null_argument = false;

	void BindColumns(RoutingBindData &data, const vector<ColumnSpec> &specs, const char *input_name);
	void AllowNulls(RoutingBindData &data, idx_t column);

	const Value &Argument(idx_t idx);
	//! Scalar integer or list of integers. Sorted and de-duplicated.
	vector<int64_t> VertexArgument(idx_t idx, const char *argument_name);
	int64_t IntegerArgument(idx_t idx, const char *argument_name);
	double DoubleArgument(idx_t idx, const char *argument_name);

	//! Options can be passed by name, or positionally after the required arguments in declaration order.
	Value Option(const char *option_name, const LogicalType &type, Value default_value);
	void Finish();
};

struct RoutingFunctionDoc {
	const char *description;
	const char *example;
};

typedef unique_ptr<FunctionData> (*routing_bind_t)(ClientContext &context, TableFunctionBindInput &input,
                                                   vector<LogicalType> &return_types, vector<string> &names);

struct NamedOption {
	const char *name;
	LogicalType type;
};

void RegisterRoutingFunction(ExtensionLoader &loader, const char *name, idx_t scalar_arguments, routing_bind_t bind,
                             const vector<NamedOption> &options, const char *description, const char *example);

void RegisterRoutingOptimizer(ExtensionLoader &loader);

void SetResultSchema(RoutingBindData &data, vector<LogicalType> &return_types, vector<string> &names,
                     const vector<std::pair<const char *, LogicalType>> &schema);

void CheckInterrupt(ClientContext &context);

} // namespace routing

} // namespace duckdb
