#include "spatial/modules/routing/routing_common.hpp"
#include "spatial/util/function_builder.hpp"

#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/common/vector_operations/vector_operations.hpp"
#include "duckdb/main/client_context.hpp"
#include "duckdb/main/extension/extension_loader.hpp"
#include "duckdb/optimizer/optimizer_extension.hpp"
#include "duckdb/planner/expression/bound_columnref_expression.hpp"
#include "duckdb/planner/operator/logical_get.hpp"
#include "duckdb/planner/operator/logical_order.hpp"

namespace duckdb {

namespace routing {

//----------------------------------------------------------------------------------------------------------------------
// Result buffer
//----------------------------------------------------------------------------------------------------------------------
void RoutingResult::Initialize(const vector<LogicalType> &types) {
	columns.clear();
	row_count = 0;
	for (auto &type : types) {
		Column column;
		column.type = type;
		switch (type.id()) {
		case LogicalTypeId::INTEGER:
		case LogicalTypeId::BIGINT:
			column.storage = Storage::INTEGER;
			break;
		case LogicalTypeId::DOUBLE:
			column.storage = Storage::DOUBLE;
			break;
		case LogicalTypeId::GEOMETRY:
		case LogicalTypeId::VARCHAR:
			column.storage = Storage::STRING;
			break;
		default:
			column.storage = Storage::VALUE;
			break;
		}
		columns.push_back(std::move(column));
	}
}

void RoutingResult::BeginRow() {
	row_count++;
	for (auto &column : columns) {
		switch (column.storage) {
		case Storage::INTEGER:
			column.integers.push_back(0);
			break;
		case Storage::DOUBLE:
			column.doubles.push_back(0);
			break;
		case Storage::STRING:
			column.strings.emplace_back();
			break;
		default:
			column.values.emplace_back(column.type);
			break;
		}
	}
}

void RoutingResult::SetInteger(idx_t col, int64_t value) {
	D_ASSERT(columns[col].storage == Storage::INTEGER);
	columns[col].integers.back() = value;
}

void RoutingResult::SetDouble(idx_t col, double value) {
	D_ASSERT(columns[col].storage == Storage::DOUBLE);
	columns[col].doubles.back() = value;
}

void RoutingResult::SetValue(idx_t col, Value value) {
	D_ASSERT(columns[col].storage == Storage::VALUE);
	columns[col].values.back() = std::move(value);
}

void RoutingResult::SetString(idx_t col, string value) {
	D_ASSERT(columns[col].storage == Storage::STRING);
	columns[col].strings.back() = std::move(value);
}

void RoutingResult::SetNull(idx_t col) {
	auto &validity = columns[col].validity;
	validity.resize(row_count, true);
	validity[row_count - 1] = false;
}

void RoutingResult::Emit(idx_t &offset, DataChunk &output) const {
	const auto count = MinValue<idx_t>(STANDARD_VECTOR_SIZE, row_count - offset);
	for (idx_t col_idx = 0; col_idx < columns.size(); col_idx++) {
		auto &column = columns[col_idx];
		auto &vector = output.data[col_idx];
		switch (column.storage) {
		case Storage::INTEGER:
			if (column.type.id() == LogicalTypeId::INTEGER) {
				auto data = FlatVector::GetData<int32_t>(vector);
				for (idx_t i = 0; i < count; i++) {
					data[i] = UnsafeNumericCast<int32_t>(column.integers[offset + i]);
				}
			} else {
				auto data = FlatVector::GetData<int64_t>(vector);
				for (idx_t i = 0; i < count; i++) {
					data[i] = column.integers[offset + i];
				}
			}
			break;
		case Storage::DOUBLE: {
			auto data = FlatVector::GetData<double>(vector);
			for (idx_t i = 0; i < count; i++) {
				data[i] = column.doubles[offset + i];
			}
		} break;
		case Storage::STRING: {
			auto data = FlatVector::GetData<string_t>(vector);
			for (idx_t i = 0; i < count; i++) {
				data[i] = StringVector::AddStringOrBlob(vector, column.strings[offset + i]);
			}
		} break;
		default:
			for (idx_t i = 0; i < count; i++) {
				vector.SetValue(i, column.values[offset + i]);
			}
			break;
		}
		for (idx_t i = 0; i < count && offset + i < column.validity.size(); i++) {
			if (!column.validity[offset + i]) {
				FlatVector::SetNull(vector, i, true);
			}
		}
	}
	output.SetCardinality(count);
	offset += count;
}

//----------------------------------------------------------------------------------------------------------------------
// Binder
//----------------------------------------------------------------------------------------------------------------------
RoutingBinder::RoutingBinder(const char *name_p, TableFunctionBindInput &input_p, idx_t required_arguments_p)
    : name(name_p), input(input_p), required_arguments(required_arguments_p), next_positional(required_arguments_p) {
	if (input.inputs.size() < required_arguments) {
		throw BinderException("%s: expected at least %llu arguments, got %llu", name, required_arguments,
		                      input.inputs.size());
	}
}

static const char *ColumnKindName(ColumnKind kind) {
	switch (kind) {
	case ColumnKind::INTEGER:
		return "an integer type";
	case ColumnKind::NUMERIC:
		return "a numeric type";
	default:
		return "GEOMETRY";
	}
}

void RoutingBinder::BindColumns(RoutingBindData &data, const vector<ColumnSpec> &specs, const char *input_name) {
	data.function_name = name;
	auto &table_names = input.input_table_names;
	auto &table_types = input.input_table_types;
	for (auto &spec : specs) {
		InputColumn column;
		column.name = spec.name;
		column.kind = spec.kind;
		for (idx_t i = 0; i < table_names.size(); i++) {
			if (!StringUtil::CIEquals(table_names[i], spec.name)) {
				continue;
			}
			auto &type = table_types[i];
			bool valid = false;
			switch (spec.kind) {
			case ColumnKind::INTEGER:
				valid = type.IsIntegral();
				break;
			case ColumnKind::NUMERIC:
				valid = type.IsNumeric();
				break;
			default:
				valid = type.id() == LogicalTypeId::GEOMETRY;
				break;
			}
			if (!valid) {
				throw BinderException("%s: column \"%s\" of the %s input must be of %s, but it is of type %s", name,
				                      spec.name, input_name, ColumnKindName(spec.kind), type.ToString());
			}
			column.present = true;
			column.input_index = i;
			column.input_type = type;
			break;
		}
		if (!column.present && spec.required) {
			string available;
			for (auto &table_name : table_names) {
				available += available.empty() ? "" : ", ";
				available += table_name;
			}
			throw BinderException("%s: the %s input is missing the required column \"%s\" (columns found: %s)", name,
			                      input_name, spec.name, available.empty() ? "none" : available);
		}
		data.columns.push_back(std::move(column));
	}
}

void RoutingBinder::AllowNulls(RoutingBindData &data, idx_t column) {
	data.columns[column].nullable = true;
}

const Value &RoutingBinder::Argument(idx_t idx) {
	auto &value = input.inputs[idx];
	if (value.IsNull()) {
		has_null_argument = true;
	}
	return value;
}

static int64_t ToBigint(const Value &value) {
	return value.DefaultCastAs(LogicalType::BIGINT).GetValue<int64_t>();
}

vector<int64_t> RoutingBinder::VertexArgument(idx_t idx, const char *argument_name) {
	vector<int64_t> result;
	auto &value = Argument(idx);
	auto &type = value.type();
	if (type.id() == LogicalTypeId::LIST || type.id() == LogicalTypeId::ARRAY) {
		if (value.IsNull()) {
			return result;
		}
		auto &children =
		    type.id() == LogicalTypeId::LIST ? ListValue::GetChildren(value) : ArrayValue::GetChildren(value);
		for (auto &child : children) {
			if (child.IsNull()) {
				continue;
			}
			if (!child.type().IsIntegral()) {
				throw BinderException("%s: argument \"%s\" must be an integer or a list of integers, got %s", name,
				                      argument_name, type.ToString());
			}
			result.push_back(ToBigint(child));
		}
		std::sort(result.begin(), result.end());
		result.erase(std::unique(result.begin(), result.end()), result.end());
		return result;
	}
	if (value.IsNull()) {
		return result;
	}
	if (!type.IsIntegral()) {
		throw BinderException("%s: argument \"%s\" must be an integer or a list of integers, got %s", name,
		                      argument_name, type.ToString());
	}
	result.push_back(ToBigint(value));
	return result;
}

int64_t RoutingBinder::IntegerArgument(idx_t idx, const char *argument_name) {
	auto &value = Argument(idx);
	if (value.IsNull()) {
		return 0;
	}
	if (!value.type().IsIntegral()) {
		throw BinderException("%s: argument \"%s\" must be an integer, got %s", name, argument_name,
		                      value.type().ToString());
	}
	return ToBigint(value);
}

double RoutingBinder::DoubleArgument(idx_t idx, const char *argument_name) {
	auto &value = Argument(idx);
	if (value.IsNull()) {
		return 0;
	}
	if (!value.type().IsNumeric()) {
		throw BinderException("%s: argument \"%s\" must be numeric, got %s", name, argument_name,
		                      value.type().ToString());
	}
	return value.DefaultCastAs(LogicalType::DOUBLE).GetValue<double>();
}

Value RoutingBinder::Option(const char *option_name, const LogicalType &type, Value default_value) {
	Value value;
	bool found = false;
	auto entry = input.named_parameters.find(option_name);
	if (entry != input.named_parameters.end()) {
		value = entry->second;
		found = true;
	} else if (next_positional < input.inputs.size()) {
		value = input.inputs[next_positional++];
		found = true;
	}
	if (!found) {
		return default_value;
	}
	if (value.IsNull()) {
		has_null_argument = true;
		return default_value;
	}
	Value result;
	string error;
	if (!value.DefaultTryCastAs(type, result, &error)) {
		throw BinderException("%s: option \"%s\" must be of type %s, got %s", name, option_name, type.ToString(),
		                      value.type().ToString());
	}
	return result;
}

void RoutingBinder::Finish() {
	if (next_positional < input.inputs.size()) {
		throw BinderException("%s: too many arguments (%llu given)", name, input.inputs.size());
	}
}

void SetResultSchema(RoutingBindData &data, vector<LogicalType> &return_types, vector<string> &names,
                     const vector<std::pair<const char *, LogicalType>> &schema) {
	for (auto &entry : schema) {
		names.emplace_back(entry.first);
		return_types.push_back(entry.second);
	}
	data.result_types = return_types;
}

void CheckInterrupt(ClientContext &context) {
	if (context.interrupted) {
		throw InterruptException();
	}
}

//----------------------------------------------------------------------------------------------------------------------
// Table in-out function
//----------------------------------------------------------------------------------------------------------------------
namespace {

struct RoutingGlobalState : public GlobalTableFunctionState {
	mutex lock;
	RoutingInput input;
	RoutingResult result;
	bool computed = false;
	idx_t offset = 0;

	// FinalExecute runs once per thread: keep the pipeline that feeds the function on a single thread
	idx_t MaxThreads() const override {
		return 1;
	}
};

unique_ptr<GlobalTableFunctionState> RoutingInitGlobal(ClientContext &context, TableFunctionInitInput &input) {
	auto &bind_data = input.bind_data->Cast<RoutingBindData>();
	auto result = make_uniq<RoutingGlobalState>();
	result->input.columns = bind_data.columns;
	result->result.Initialize(bind_data.result_types);
	return std::move(result);
}

void AppendColumn(ClientContext &context, const string &function_name, InputColumn &column, DataChunk &input) {
	const auto count = input.size();
	auto &source = input.data[column.input_index];

	LogicalType target_type;
	switch (column.kind) {
	case ColumnKind::INTEGER:
		target_type = LogicalType::BIGINT;
		break;
	case ColumnKind::NUMERIC:
		target_type = LogicalType::DOUBLE;
		break;
	default:
		target_type = source.GetType();
		break;
	}

	Vector casted(target_type);
	reference<Vector> vector_ref(source);
	if (source.GetType() != target_type) {
		VectorOperations::Cast(context, source, casted, count);
		vector_ref = casted;
	}

	UnifiedVectorFormat format;
	vector_ref.get().ToUnifiedFormat(count, format);

	for (idx_t i = 0; i < count; i++) {
		const auto idx = format.sel->get_index(i);
		const auto valid = format.validity.RowIsValid(idx);
		if (!valid && !column.nullable) {
			throw InvalidInputException("%s: unexpected NULL value in column \"%s\"", function_name, column.name);
		}
		if (column.nullable) {
			column.validity.push_back(valid);
		}
		switch (column.kind) {
		case ColumnKind::INTEGER:
			column.integers.push_back(valid ? UnifiedVectorFormat::GetData<int64_t>(format)[idx] : 0);
			break;
		case ColumnKind::NUMERIC:
			column.numerics.push_back(valid ? UnifiedVectorFormat::GetData<double>(format)[idx] : 0);
			break;
		default:
			column.blobs.push_back(valid ? UnifiedVectorFormat::GetData<string_t>(format)[idx].GetString()
			                             : string());
			break;
		}
	}
}

OperatorResultType RoutingInOut(ExecutionContext &context, TableFunctionInput &data, DataChunk &input,
                                DataChunk &output) {
	auto &bind_data = data.bind_data->Cast<RoutingBindData>();
	auto &state = data.global_state->Cast<RoutingGlobalState>();

	lock_guard<mutex> guard(state.lock);
	if (state.computed) {
		throw InvalidInputException("%s: the table argument arrived in several parts, which is only supported when "
		                            "the optimizer and its extensions are enabled",
		                            bind_data.function_name);
	}
	for (auto &column : state.input.columns) {
		if (column.present) {
			AppendColumn(context.client, bind_data.function_name, column, input);
		}
	}
	state.input.row_count += input.size();
	output.SetCardinality(0);
	return OperatorResultType::NEED_MORE_INPUT;
}

OperatorFinalizeResultType RoutingFinal(ExecutionContext &context, TableFunctionInput &data, DataChunk &output) {
	auto &bind_data = data.bind_data->Cast<RoutingBindData>();
	auto &state = data.global_state->Cast<RoutingGlobalState>();

	lock_guard<mutex> guard(state.lock);
	if (!state.computed) {
		bind_data.Compute(context.client, state.input, state.result);
		state.computed = true;
		state.input = RoutingInput();
	}
	if (state.offset >= state.result.RowCount()) {
		output.SetCardinality(0);
		return OperatorFinalizeResultType::FINISHED;
	}
	state.result.Emit(state.offset, output);
	return OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// The result can only be computed once the whole input has been seen, but a table in-out function is flushed at the
// end of every pipeline that feeds it, and an input such as a UNION ALL arrives through several pipelines. Sorting the
// input first makes it arrive through a single pipeline, after all of it has been produced.
void InsertInputBarrier(OptimizerExtensionInput &input, unique_ptr<LogicalOperator> &plan) {
	for (auto &child : plan->children) {
		InsertInputBarrier(input, child);
	}
	if (plan->type != LogicalOperatorType::LOGICAL_GET || plan->children.size() != 1) {
		return;
	}
	if (plan->Cast<LogicalGet>().function.in_out_function != RoutingInOut) {
		return;
	}
	auto &child = plan->children[0];
	if (child->type == LogicalOperatorType::LOGICAL_ORDER_BY) {
		return;
	}
	child->ResolveOperatorTypes();
	const auto bindings = child->GetColumnBindings();
	if (bindings.empty()) {
		return;
	}
	idx_t column = 0;
	for (idx_t i = 0; i < child->types.size(); i++) {
		if (child->types[i].IsNumeric()) {
			column = i;
			break;
		}
	}
	vector<BoundOrderByNode> orders;
	orders.emplace_back(OrderType::ASCENDING, OrderByNullType::NULLS_LAST,
	                    make_uniq<BoundColumnRefExpression>(child->types[column], bindings[column]));
	auto order = make_uniq<LogicalOrder>(std::move(orders));
	if (child->has_estimated_cardinality) {
		order->SetEstimatedCardinality(child->estimated_cardinality);
	}
	order->children.push_back(std::move(child));
	plan->children[0] = std::move(order);
}

} // namespace

void RegisterRoutingOptimizer(ExtensionLoader &loader) {
	OptimizerExtension optimizer;
	optimizer.optimize_function = InsertInputBarrier;
	OptimizerExtension::Register(loader.GetDatabaseInstance().config, optimizer);
}

void RegisterRoutingFunction(ExtensionLoader &loader, const char *name, idx_t scalar_arguments, routing_bind_t bind,
                             const vector<NamedOption> &options, const char *description, const char *example) {
	vector<LogicalType> arguments;
	arguments.push_back(LogicalType::TABLE);
	for (idx_t i = 0; i < scalar_arguments; i++) {
		arguments.push_back(LogicalType::ANY);
	}
	TableFunction function(name, arguments, nullptr, bind, RoutingInitGlobal);
	function.in_out_function = RoutingInOut;
	function.in_out_function_final = RoutingFinal;
	if (!options.empty()) {
		function.varargs = LogicalType::ANY;
	}
	for (auto &option : options) {
		function.named_parameters[option.name] = option.type;
	}
	loader.RegisterFunction(function);

	InsertionOrderPreservingMap<string> tags;
	tags.insert("ext", "spatial");
	tags.insert("category", "routing");
	FunctionBuilder::AddTableFunctionDocs(loader, name, description, example, tags);
}

} // namespace routing

} // namespace duckdb
