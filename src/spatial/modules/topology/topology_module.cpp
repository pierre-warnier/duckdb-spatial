#include "spatial/modules/topology/topology_module.hpp"

#include "duckdb/common/string_util.hpp"
#include "duckdb/common/types/geometry_crs.hpp"
#include "duckdb/function/table_function.hpp"
#include "duckdb/parser/keyword_helper.hpp"
#include "duckdb/main/extension/extension_loader.hpp"

#include "spatial/modules/topology/topology_ops.hpp"
#include "spatial/util/function_builder.hpp"

namespace duckdb {

namespace {

using topology::TopoRows;
using topology::TopoSession;

//======================================================================================================================
// Table function plumbing
//======================================================================================================================

struct TopoCall {
	vector<Value> args;
	//! Identifier of the coordinate system attached to the type of each argument, empty if none
	vector<string> crs;
};

typedef void (*topo_function_t)(ClientContext &context, const TopoCall &call, TopoRows &rows);

struct TopoFunctionInfo final : public TableFunctionInfo {
	topo_function_t function;
	string name;
	//! Declared argument types of the overload with the most arguments
	vector<LogicalType> arguments;
	vector<string> names;
	vector<LogicalType> types;
};

struct TopoBindData final : public TableFunctionData {
	topo_function_t function;
	TopoCall call;
};

struct TopoGlobalState final : public GlobalTableFunctionState {
	bool executed = false;
	TopoRows rows;
	idx_t offset = 0;
};

unique_ptr<FunctionData> Bind(ClientContext &context, TableFunctionBindInput &input, vector<LogicalType> &return_types,
                              vector<string> &names) {
	auto &info = input.info->Cast<TopoFunctionInfo>();
	auto result = make_uniq<TopoBindData>();
	result->function = info.function;
	for (idx_t i = 0; i < input.inputs.size(); i++) {
		auto value = input.inputs[i];
		string crs;
		if (info.arguments[i].id() == LogicalTypeId::GEOMETRY) {
			// Geometry arguments are registered as ANY: a GEOMETRY parameter would cast away the coordinate system
			if (value.IsNull()) {
				value = Value(LogicalType::GEOMETRY());
			} else if (value.type().id() == LogicalTypeId::GEOMETRY) {
				if (GeoType::HasCRS(value.type())) {
					crs = GeoType::GetCRS(value.type()).GetIdentifier();
				}
			} else if (value.type().id() == LogicalTypeId::VARCHAR) {
				value = value.CastAs(context, LogicalType::GEOMETRY());
			} else {
				throw BinderException("%s: argument %d must be a GEOMETRY, not %s", info.name, i + 1,
				                      value.type().ToString());
			}
		}
		result->call.args.push_back(std::move(value));
		result->call.crs.push_back(std::move(crs));
	}
	return_types = info.types;
	names = info.names;
	return std::move(result);
}

unique_ptr<GlobalTableFunctionState> InitGlobal(ClientContext &context, TableFunctionInitInput &input) {
	return make_uniq<TopoGlobalState>();
}

//! The function body runs on the first scan, so that binding alone (EXPLAIN, PREPARE) never touches the topology
void Scan(ClientContext &context, TableFunctionInput &input, DataChunk &output) {
	auto &bind_data = input.bind_data->Cast<TopoBindData>();
	auto &state = input.global_state->Cast<TopoGlobalState>();
	if (!state.executed) {
		state.executed = true;
		bind_data.function(context, bind_data.call, state.rows);
	}
	const auto count = MinValue<idx_t>(STANDARD_VECTOR_SIZE, state.rows.size() - state.offset);
	for (idx_t row = 0; row < count; row++) {
		auto &values = state.rows[state.offset + row];
		for (idx_t col = 0; col < values.size(); col++) {
			output.SetValue(col, row, values[col]);
		}
	}
	state.offset += count;
	output.SetCardinality(count);
}

int32_t IntArg(const TopoCall &call, idx_t idx) {
	return call.args[idx].GetValue<int32_t>();
}

void RequireAll(const TopoCall &call) {
	topology::RequireArguments(call.args);
}

void AddSequenceRows(const vector<int32_t> &edges, TopoRows &rows) {
	for (idx_t i = 0; i < edges.size(); i++) {
		rows.push_back({Value::INTEGER(NumericCast<int32_t>(i + 1)), Value::INTEGER(edges[i])});
	}
}

//======================================================================================================================
// Management
//======================================================================================================================

void ExecCreateTopology(ClientContext &context, const TopoCall &call, TopoRows &rows) {
	RequireAll(call);
	const auto name = call.args[0].GetValue<string>();
	const auto srid = call.args.size() > 1 ? IntArg(call, 1) : 0;
	const auto precision = call.args.size() > 2 ? call.args[2].GetValue<double>() : 0.0;
	const auto hasz = call.args.size() > 3 && call.args[3].GetValue<bool>();
	TopoSession::CheckName(name);
	if (srid < 0) {
		throw InvalidInputException("SRID must be a non-negative integer");
	}
	if (!(precision >= 0)) {
		throw InvalidInputException("Precision must be a non-negative number");
	}
	if (hasz) {
		throw NotImplementedException("Topologies with Z coordinates are not supported");
	}

	TopoSession session(context, true);
	session.CreateRegistry();
	if (session.TryOpenTopology(name)) {
		throw InvalidInputException("Topology \"%s\" already exists", session.Info().name);
	}
	auto schemas =
	    session.Query("SELECT count(*) FROM duckdb_schemas() WHERE database_name = ? AND lower(schema_name) = lower(?)",
	                  {Value(session.Catalog()), Value(name)});
	if (schemas->GetValue(0, 0).GetValue<int64_t>() > 0) {
		throw InvalidInputException("Schema \"%s\" already exists", name);
	}

	const auto catalog = KeywordHelper::WriteQuoted(session.Catalog(), '"');
	const auto sequence = KeywordHelper::WriteQuoted(catalog + ".topology.topology_id_seq", '\'');
	auto id_result = session.Query("SELECT nextval(" + sequence + ")");
	const auto id = id_result->GetValue(0, 0).GetValue<int32_t>();
	session.Query("INSERT INTO " + session.Registry() +
	                  "(id, name, srid, \"precision\", hasz) VALUES (?, ?, ?, ?, false)",
	              {Value::INTEGER(id), Value(name), Value::INTEGER(srid), Value::DOUBLE(precision)});

	const auto schema = TopoSession::SchemaFor(session.Catalog(), name);
	const auto geometry = srid > 0 ? "GEOMETRY('EPSG:" + to_string(srid) + "')" : string("GEOMETRY");
	auto next_id = [&](const char *sequence_name) {
		return "DEFAULT nextval(" + KeywordHelper::WriteQuoted(name + "." + sequence_name, '\'') + ")";
	};
	session.Execute("CREATE SCHEMA " + schema);
	session.Execute("CREATE SEQUENCE " + schema + ".node_node_id_seq");
	session.Execute("CREATE SEQUENCE " + schema + ".edge_data_edge_id_seq");
	session.Execute("CREATE SEQUENCE " + schema + ".face_face_id_seq");
	session.Execute("CREATE TABLE " + schema + ".face(face_id INTEGER PRIMARY KEY " + next_id("face_face_id_seq") +
	                ", mbr " + geometry + ")");
	session.Execute("CREATE TABLE " + schema + ".node(node_id INTEGER PRIMARY KEY " + next_id("node_node_id_seq") +
	                ", containing_face INTEGER, geom " + geometry + " NOT NULL)");
	session.Execute("CREATE TABLE " + schema + ".edge_data(edge_id INTEGER PRIMARY KEY " +
	                next_id("edge_data_edge_id_seq") +
	                ", start_node INTEGER NOT NULL, end_node INTEGER NOT NULL, next_left_edge INTEGER NOT NULL, "
	                "abs_next_left_edge INTEGER NOT NULL, next_right_edge INTEGER NOT NULL, "
	                "abs_next_right_edge INTEGER NOT NULL, left_face INTEGER NOT NULL, right_face INTEGER NOT NULL, "
	                "geom " +
	                geometry + " NOT NULL)");
	session.Execute("CREATE VIEW " + schema +
	                ".edge AS SELECT edge_id, start_node, end_node, next_left_edge, next_right_edge, left_face, "
	                "right_face, geom FROM " +
	                schema + ".edge_data");
	session.Execute("INSERT INTO " + schema + ".face(face_id, mbr) VALUES (0, NULL)");
	session.Commit();
	rows.push_back({Value::INTEGER(id)});
}

void ExecDropTopology(ClientContext &context, const TopoCall &call, TopoRows &rows) {
	TopoSession session(context, true);
	session.OpenTopology(call.args[0]);
	const auto name = session.Info().name;
	session.Query("DELETE FROM " + session.Registry() + " WHERE id = ?", {Value::INTEGER(session.Info().id)});
	session.Execute("DROP SCHEMA " + TopoSession::SchemaFor(session.Catalog(), name) + " CASCADE");
	session.Commit();
	rows.push_back({Value("Topology '" + name + "' dropped")});
}

void ExecGetTopologyID(ClientContext &context, const TopoCall &call, TopoRows &rows) {
	TopoSession session(context, false);
	Value result(LogicalType::INTEGER);
	if (!call.args[0].IsNull() && session.TryOpenTopology(call.args[0].GetValue<string>())) {
		result = Value::INTEGER(session.Info().id);
	}
	rows.push_back({result});
}

void ExecGetTopologySRID(ClientContext &context, const TopoCall &call, TopoRows &rows) {
	TopoSession session(context, false);
	Value result(LogicalType::INTEGER);
	if (!call.args[0].IsNull() && session.TryOpenTopology(call.args[0].GetValue<string>())) {
		result = Value::INTEGER(session.Info().srid);
	}
	rows.push_back({result});
}

void ExecGetTopologyName(ClientContext &context, const TopoCall &call, TopoRows &rows) {
	TopoSession session(context, false);
	Value result(LogicalType::VARCHAR);
	if (!call.args[0].IsNull() && session.RegistryExists()) {
		auto names = session.Query("SELECT name FROM " + session.Registry() + " WHERE id = ?", {call.args[0]});
		if (names->RowCount() > 0) {
			result = names->GetValue(0, 0);
		}
	}
	rows.push_back({result});
}

void ExecTopologySummary(ClientContext &context, const TopoCall &call, TopoRows &rows) {
	TopoSession session(context, false);
	session.OpenTopology(call.args[0]);
	auto counts = session.Query("SELECT (SELECT count(*) FROM " + session.Table("node") + "), (SELECT count(*) FROM " +
	                            session.Table("edge_data") + "), (SELECT count(*) FROM " + session.Table("face") +
	                            " WHERE face_id <> 0)");
	auto &info = session.Info();
	const auto summary = StringUtil::Format(
	    "Topology %s (id %d, SRID %d, precision %s)\n%lld nodes, %lld edges, %lld faces, 0 topogeoms "
	    "in 0 layers",
	    info.name, info.id, info.srid, topology::FormatNumber(info.precision),
	    counts->GetValue(0, 0).GetValue<int64_t>(), counts->GetValue(1, 0).GetValue<int64_t>(),
	    counts->GetValue(2, 0).GetValue<int64_t>());
	rows.push_back({Value(summary)});
}

//======================================================================================================================
// Population and editing
//======================================================================================================================

void ExecCreateTopoGeo(ClientContext &context, const TopoCall &call, TopoRows &rows) {
	RequireAll(call);
	TopoSession session(context, true);
	session.OpenTopology(call.args[0]);
	session.CheckSRID(call.crs[1]);
	topology::CreateTopoGeo(session, call.args[1]);
	session.Commit();
	rows.push_back({Value("Topology " + session.Info().name + " populated")});
}

void ExecAddIsoNode(ClientContext &context, const TopoCall &call, TopoRows &rows) {
	topology::RequireArguments({call.args[0], call.args[2]});
	TopoSession session(context, true);
	session.OpenTopology(call.args[0]);
	session.CheckSRID(call.crs[2]);
	const auto id = topology::AddIsoNode(session, call.args[1], call.args[2]);
	session.Commit();
	rows.push_back({Value::INTEGER(id)});
}

void ExecAddIsoEdge(ClientContext &context, const TopoCall &call, TopoRows &rows) {
	RequireAll(call);
	TopoSession session(context, true);
	session.OpenTopology(call.args[0]);
	session.CheckSRID(call.crs[3]);
	const auto id = topology::AddIsoEdge(session, IntArg(call, 1), IntArg(call, 2), call.args[3]);
	session.Commit();
	rows.push_back({Value::INTEGER(id)});
}

template <bool MOD_FACE>
void ExecAddEdge(ClientContext &context, const TopoCall &call, TopoRows &rows) {
	RequireAll(call);
	TopoSession session(context, true);
	session.OpenTopology(call.args[0]);
	session.CheckSRID(call.crs[3]);
	const auto id = topology::AddEdge(session, IntArg(call, 1), IntArg(call, 2), call.args[3], MOD_FACE);
	session.Commit();
	rows.push_back({Value::INTEGER(id)});
}

template <bool MOD_FACE>
void ExecRemEdge(ClientContext &context, const TopoCall &call, TopoRows &rows) {
	RequireAll(call);
	TopoSession session(context, true);
	session.OpenTopology(call.args[0]);
	const auto face = topology::RemoveEdge(session, IntArg(call, 1), MOD_FACE);
	session.Commit();
	rows.push_back({face});
}

void ExecChangeEdgeGeom(ClientContext &context, const TopoCall &call, TopoRows &rows) {
	RequireAll(call);
	TopoSession session(context, true);
	session.OpenTopology(call.args[0]);
	session.CheckSRID(call.crs[2]);
	topology::ChangeEdgeGeom(session, IntArg(call, 1), call.args[2]);
	session.Commit();
	rows.push_back({Value(StringUtil::Format("Edge %d changed", IntArg(call, 1)))});
}

template <bool MOD_EDGE>
void ExecSplitEdge(ClientContext &context, const TopoCall &call, TopoRows &rows) {
	RequireAll(call);
	TopoSession session(context, true);
	session.OpenTopology(call.args[0]);
	session.CheckSRID(call.crs[2]);
	const auto node = topology::SplitEdge(session, IntArg(call, 1), call.args[2], MOD_EDGE);
	session.Commit();
	rows.push_back({Value::INTEGER(node)});
}

template <bool MOD_EDGE>
void ExecHealEdges(ClientContext &context, const TopoCall &call, TopoRows &rows) {
	RequireAll(call);
	TopoSession session(context, true);
	session.OpenTopology(call.args[0]);
	const auto id = topology::HealEdges(session, IntArg(call, 1), IntArg(call, 2), MOD_EDGE);
	session.Commit();
	rows.push_back({Value::INTEGER(id)});
}

void ExecMoveIsoNode(ClientContext &context, const TopoCall &call, TopoRows &rows) {
	RequireAll(call);
	TopoSession session(context, true);
	session.OpenTopology(call.args[0]);
	session.CheckSRID(call.crs[2]);
	topology::MoveIsoNode(session, IntArg(call, 1), call.args[2]);
	const auto p = topology::ParsePoint(session, call.args[2]);
	session.Commit();
	rows.push_back({Value(StringUtil::Format("Isolated Node %d moved to location %s,%s", IntArg(call, 1),
	                                         topology::FormatNumber(p.x), topology::FormatNumber(p.y)))});
}

void ExecRemoveIsoNode(ClientContext &context, const TopoCall &call, TopoRows &rows) {
	RequireAll(call);
	TopoSession session(context, true);
	session.OpenTopology(call.args[0]);
	topology::RemoveIsoNode(session, IntArg(call, 1));
	session.Commit();
	rows.push_back({Value(StringUtil::Format("Isolated node %d removed", IntArg(call, 1)))});
}

void ExecRemoveIsoEdge(ClientContext &context, const TopoCall &call, TopoRows &rows) {
	RequireAll(call);
	TopoSession session(context, true);
	session.OpenTopology(call.args[0]);
	topology::RemoveIsoEdge(session, IntArg(call, 1));
	session.Commit();
	rows.push_back({Value(StringUtil::Format("Isolated edge %d removed", IntArg(call, 1)))});
}

//======================================================================================================================
// Accessors and validation
//======================================================================================================================

void ExecGetFaceGeometry(ClientContext &context, const TopoCall &call, TopoRows &rows) {
	RequireAll(call);
	TopoSession session(context, false);
	session.OpenTopology(call.args[0]);
	rows.push_back({topology::GetFaceGeometry(session, IntArg(call, 1))});
}

void ExecGetFaceEdges(ClientContext &context, const TopoCall &call, TopoRows &rows) {
	RequireAll(call);
	TopoSession session(context, false);
	session.OpenTopology(call.args[0]);
	AddSequenceRows(topology::GetFaceEdges(session, IntArg(call, 1)), rows);
}

typedef int32_t (*point_lookup_t)(TopoSession &session, const Value &point, double tolerance);

template <point_lookup_t LOOKUP>
void ExecPointLookup(ClientContext &context, const TopoCall &call, TopoRows &rows) {
	RequireAll(call);
	TopoSession session(context, false);
	session.OpenTopology(call.args[0]);
	session.CheckSRID(call.crs[1]);
	rows.push_back({Value::INTEGER(LOOKUP(session, call.args[1], call.args[2].GetValue<double>()))});
}

void ExecGetNodeEdges(ClientContext &context, const TopoCall &call, TopoRows &rows) {
	RequireAll(call);
	TopoSession session(context, false);
	session.OpenTopology(call.args[0]);
	AddSequenceRows(topology::GetNodeEdges(session, IntArg(call, 1)), rows);
}

void ExecGetRingEdges(ClientContext &context, const TopoCall &call, TopoRows &rows) {
	topology::RequireArguments({call.args[0], call.args[1]});
	TopoSession session(context, false);
	session.OpenTopology(call.args[0]);
	const auto limit = call.args.size() > 2 ? call.args[2] : Value(LogicalType::INTEGER);
	AddSequenceRows(topology::GetRingEdges(session, IntArg(call, 1), limit), rows);
}

void ExecValidateTopology(ClientContext &context, const TopoCall &call, TopoRows &rows) {
	RequireAll(call);
	TopoSession session(context, false);
	session.OpenTopology(call.args[0]);
	topology::ValidateTopology(session, rows);
}

//======================================================================================================================
// TopoElementArray_Agg
//======================================================================================================================

struct TopoElementAggState {
	vector<pair<int32_t, int32_t>> *elements;
};

struct TopoElementAggFunction {
	template <class STATE>
	static void Initialize(STATE &state) {
		state.elements = nullptr;
	}

	template <class STATE, class OP>
	static void Combine(const STATE &source, STATE &target, AggregateInputData &) {
		if (!source.elements) {
			return;
		}
		if (!target.elements) {
			target.elements = new vector<pair<int32_t, int32_t>>();
		}
		target.elements->insert(target.elements->end(), source.elements->begin(), source.elements->end());
	}

	template <class STATE>
	static void Destroy(STATE &state, AggregateInputData &) {
		delete state.elements;
		state.elements = nullptr;
	}

	static void Update(Vector inputs[], AggregateInputData &, idx_t, Vector &state_vector, idx_t count) {
		auto &input = inputs[0];
		UnifiedVectorFormat input_format;
		input.ToUnifiedFormat(count, input_format);
		auto &child = ArrayVector::GetEntry(input);
		UnifiedVectorFormat child_format;
		child.ToUnifiedFormat(ArrayVector::GetTotalSize(input), child_format);
		const auto child_data = UnifiedVectorFormat::GetData<int32_t>(child_format);

		UnifiedVectorFormat state_format;
		state_vector.ToUnifiedFormat(count, state_format);
		const auto states = UnifiedVectorFormat::GetData<TopoElementAggState *>(state_format);

		for (idx_t i = 0; i < count; i++) {
			const auto row = input_format.sel->get_index(i);
			if (!input_format.validity.RowIsValid(row)) {
				continue;
			}
			const auto id_idx = child_format.sel->get_index(row * 2);
			const auto type_idx = child_format.sel->get_index(row * 2 + 1);
			if (!child_format.validity.RowIsValid(id_idx) || !child_format.validity.RowIsValid(type_idx)) {
				throw InvalidInputException("TopoElementArray_Agg: a TopoElement cannot hold NULL values");
			}
			auto &state = *states[state_format.sel->get_index(i)];
			if (!state.elements) {
				state.elements = new vector<pair<int32_t, int32_t>>();
			}
			state.elements->emplace_back(child_data[id_idx], child_data[type_idx]);
		}
	}

	static void Finalize(Vector &state_vector, AggregateInputData &, Vector &result, idx_t count, idx_t offset) {
		UnifiedVectorFormat state_format;
		state_vector.ToUnifiedFormat(count, state_format);
		const auto states = UnifiedVectorFormat::GetData<TopoElementAggState *>(state_format);

		const auto entries = FlatVector::GetData<list_entry_t>(result);
		auto &validity = FlatVector::Validity(result);
		auto total = ListVector::GetListSize(result);
		for (idx_t i = 0; i < count; i++) {
			auto &state = *states[state_format.sel->get_index(i)];
			const auto row = i + offset;
			if (!state.elements || state.elements->empty()) {
				validity.SetInvalid(row);
				continue;
			}
			const auto length = state.elements->size();
			ListVector::Reserve(result, total + length);
			auto &arrays = ListVector::GetEntry(result);
			const auto values = FlatVector::GetData<int32_t>(ArrayVector::GetEntry(arrays));
			for (idx_t k = 0; k < length; k++) {
				values[(total + k) * 2] = (*state.elements)[k].first;
				values[(total + k) * 2 + 1] = (*state.elements)[k].second;
			}
			entries[row] = list_entry_t {total, length};
			total += length;
		}
		ListVector::SetListSize(result, total);
	}
};

void RegisterTopoElementArrayAgg(ExtensionLoader &loader) {
	FunctionBuilder::RegisterAggregate(loader, "TopoElementArray_Agg", [](AggregateFunctionBuilder &func) {
		const auto element = LogicalType::ARRAY(LogicalType::INTEGER, 2);
		AggregateFunction agg("TopoElementArray_Agg", {element}, LogicalType::LIST(element),
		                      AggregateFunction::StateSize<TopoElementAggState>,
		                      AggregateFunction::StateInitialize<TopoElementAggState, TopoElementAggFunction>,
		                      TopoElementAggFunction::Update,
		                      AggregateFunction::StateCombine<TopoElementAggState, TopoElementAggFunction>,
		                      TopoElementAggFunction::Finalize, nullptr, nullptr,
		                      AggregateFunction::StateDestroy<TopoElementAggState, TopoElementAggFunction>);
		func.SetFunction(agg);
		func.SetDescription(R"(
			Collects TopoElements into a TopoElementArray.

			A TopoElement is an `INTEGER[2]` holding `[element_id, element_type]`, where the type is 1 for a node,
			2 for an edge and 3 for a face. An `INTEGER[]` list such as `[face_id, 3]` is cast implicitly and must
			hold exactly two values. The result is an `INTEGER[2][]` with one entry per input row, in input order
			(use `ORDER BY` inside the call to control it). NULL rows are skipped and the result is NULL when no
			row is aggregated. An element holding a NULL raises an error.

			Unlike PostGIS there are no `TopoElement` and `TopoElementArray` domain types: plain integer arrays are
			used and the element type is not range-checked.
		)");
		func.SetExample(R"(
			SELECT TopoElementArray_Agg([face_id, 3] ORDER BY face_id) FROM (VALUES (1), (2), (3)) t(face_id);
			-- [[1, 3], [2, 3], [3, 3]]
		)");
		func.SetTag("ext", "spatial");
		func.SetTag("category", "topology");
	});
}

//======================================================================================================================
// Registration
//======================================================================================================================

const char *const EDIT_NOTE = R"(
	The function runs in its own transaction, on a separate connection to the same database. The change is committed as soon as the call returns, independently of the transaction of the caller: a later `ROLLBACK` does not undo it. If the call fails nothing is changed. It cannot see uncommitted changes either. Calling it inside an explicit transaction (`BEGIN`) that already holds uncommitted changes to the database therefore raises an error asking to commit or roll back first; the check covers any change to the database, not only changes to the tables of the topology, because DuckDB does not expose which tables a transaction has updated or deleted from. A conflict with a concurrent transaction that modified the same topology rows is reported as an error too. Inside an explicit transaction the caller may not see the change in the topology tables until it starts a new transaction. Concurrent edits of one database are executed one after the other.

	Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`; subqueries and column references are not allowed. Geometry arguments take a `GEOMETRY` (or its WKT text); a geometry whose type carries a coordinate system must match the SRID of the topology.)";

const char *const READ_NOTE = R"(
	The function reads through a separate connection and sees committed data only. Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`.)";

struct TopoFunctionSpec {
	const char *name;
	vector<vector<LogicalType>> overloads;
	vector<string> names;
	vector<LogicalType> types;
	topo_function_t function;
	const char *note;
	const char *description;
	const char *example;
};

void RegisterTopoFunction(ExtensionLoader &loader, const TopoFunctionSpec &spec) {
	auto info = make_shared_ptr<TopoFunctionInfo>();
	info->function = spec.function;
	info->name = spec.name;
	info->names = spec.names;
	info->types = spec.types;
	for (auto &arguments : spec.overloads) {
		if (arguments.size() > info->arguments.size()) {
			info->arguments = arguments;
		}
	}

	TableFunctionSet set(spec.name);
	for (auto arguments : spec.overloads) {
		for (auto &type : arguments) {
			if (type.id() == LogicalTypeId::GEOMETRY) {
				type = LogicalType::ANY;
			}
		}
		TableFunction function(spec.name, arguments, Scan, Bind, InitGlobal);
		function.function_info = info;
		set.AddFunction(function);
	}
	loader.RegisterFunction(set);

	InsertionOrderPreservingMap<string> tags;
	tags.insert("ext", "spatial");
	tags.insert("category", "topology");
	const auto description = FunctionBuilder::RemoveIndentAndTrailingWhitespace(spec.description) + "\n\n" +
	                         FunctionBuilder::RemoveIndentAndTrailingWhitespace(spec.note);
	FunctionBuilder::AddTableFunctionDocs(loader, spec.name, description.c_str(), spec.example, tags);
}

} // namespace

void RegisterTopologyModule(ExtensionLoader &loader) {
	const LogicalType varchar = LogicalType::VARCHAR;
	const LogicalType integer = LogicalType::INTEGER;
	const LogicalType real = LogicalType::DOUBLE;
	const LogicalType boolean = LogicalType::BOOLEAN;
	const LogicalType geometry = LogicalType::GEOMETRY();
	const vector<string> sequence_names = {"sequence", "edge"};
	const vector<LogicalType> sequence_types = {integer, integer};

	RegisterTopoFunction(loader,
	                     {"CreateTopology",
	                      {{varchar}, {varchar, integer}, {varchar, integer, real}, {varchar, integer, real, boolean}},
	                      {"createtopology"},
	                      {integer},
	                      ExecCreateTopology,
	                      EDIT_NOTE,
	                      R"(
	        Creates a new, empty topology and returns its id.

	        `CreateTopology(name, srid := 0, precision := 0, hasz := false)` creates a schema called `name` in the current database, holding the PostGIS topology tables `node(node_id, containing_face, geom)`, `edge_data(edge_id, start_node, end_node, next_left_edge, abs_next_left_edge, next_right_edge, abs_next_right_edge, left_face, right_face, geom)` and `face(face_id, mbr)` with the universal face `0`, the view `edge`, and the sequences `node_node_id_seq`, `edge_data_edge_id_seq` and `face_face_id_seq`. The topology is recorded in `topology.topology(id, name, srid, precision, hasz)`, which is created on first use. When `srid` is positive the geometry columns are typed `GEOMETRY('EPSG:<srid>')`.

	        Errors: the name is not a plain identifier (letters, digits and underscores, not starting with a digit), or a topology or schema with that name already exists; negative `srid` or `precision`; `hasz = true`.

	        Differences from PostGIS: the function is not schema-qualified (`CreateTopology`, not `topology.CreateTopology`), topologies are two-dimensional only, there are no foreign keys between the topology tables, and the `topology.layer` table and the `relation` table of the TopoGeometry layer are not created.
	     )",
	                      R"(
	        CALL CreateTopology('city', 31370);
	        SELECT * FROM topology.topology;
	     )"});

	RegisterTopoFunction(loader, {"DropTopology",
	                              {{varchar}},
	                              {"droptopology"},
	                              {varchar},
	                              ExecDropTopology,
	                              EDIT_NOTE,
	                              R"(
	        Drops a topology: its schema with everything in it, and its row in `topology.topology`. Returns the text `Topology 'name' dropped`.

	        Errors: `SQL/MM Spatial exception - invalid topology name` if the topology is not registered. Unlike PostGIS, a schema that is not a registered topology is never dropped.
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        CALL DropTopology('city');
	     )"});

	RegisterTopoFunction(loader, {"GetTopologyID",
	                              {{varchar}},
	                              {"gettopologyid"},
	                              {integer},
	                              ExecGetTopologyID,
	                              READ_NOTE,
	                              R"(
	        Returns the id of the topology with the given name, or NULL if there is none.
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        SELECT * FROM GetTopologyID('city');
	     )"});

	RegisterTopoFunction(loader, {"GetTopologySRID",
	                              {{varchar}},
	                              {"gettopologysrid"},
	                              {integer},
	                              ExecGetTopologySRID,
	                              READ_NOTE,
	                              R"(
	        Returns the SRID the topology with the given name was created with, or NULL if there is no such topology.
	     )",
	                              R"(
	        CALL CreateTopology('city', 31370);
	        SELECT * FROM GetTopologySRID('city');
	     )"});

	RegisterTopoFunction(loader, {"GetTopologyName",
	                              {{integer}},
	                              {"gettopologyname"},
	                              {varchar},
	                              ExecGetTopologyName,
	                              READ_NOTE,
	                              R"(
	        Returns the name of the topology with the given id, or NULL if there is none.
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        SET VARIABLE id = (SELECT * FROM GetTopologyID('city'));
	        SELECT * FROM GetTopologyName(getvariable('id'));
	        -- city
	     )"});

	RegisterTopoFunction(loader, {"TopologySummary",
	                              {{varchar}},
	                              {"topologysummary"},
	                              {varchar},
	                              ExecTopologySummary,
	                              READ_NOTE,
	                              R"(
	        Returns a two-line text summary of a topology: its id, SRID and precision, then the number of nodes, edges and faces (the universal face is not counted).

	        Errors: `SQL/MM Spatial exception - invalid topology name`. The TopoGeometry layer is not implemented, so the summary always reports `0 topogeoms in 0 layers`.
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        SELECT * FROM TopologySummary('city');
	        -- Topology city (id 1, SRID 0, precision 0)
	        -- 0 nodes, 0 edges, 0 faces, 0 topogeoms in 0 layers
	     )"});

	RegisterTopoFunction(loader, {"ST_CreateTopoGeo",
	                              {{varchar, geometry}},
	                              {"st_createtopogeo"},
	                              {varchar},
	                              ExecCreateTopoGeo,
	                              EDIT_NOTE,
	                              R"(
	        Populates an empty topology from a geometry collection and returns the text `Topology name populated`.

	        `ST_CreateTopoGeo(toponame, collection)` takes the lines and polygon boundaries of the collection, nodes them against each other, merges the result into maximal edges and cuts these at the input points and at the end points of the input lines. It then stores the nodes (input points that are on no edge become isolated nodes), the edges with their `next_left_edge` / `next_right_edge` links, and one face per bounded region with its `left_face` / `right_face` labels and bounding box. Z and M coordinates are dropped.

	        Errors: `SQL/MM Spatial exception - null argument`, `SQL/MM Spatial exception - invalid topology name`, `SQL/MM Spatial exception - non-empty view` (the topology already holds nodes or edges), `SQL/MM Spatial exception - non-empty face view`, `Geometry SRID (...) does not match topology SRID (...)`.

	        Differences from PostGIS: the whole topology is computed in memory and written at once instead of edge by edge, so identifiers are assigned in a different order (edges are sorted by their coordinates); a closed ring keeps a single node, placed on an input point if one lies on it.
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText(
	            'GEOMETRYCOLLECTION(POLYGON((0 0, 10 0, 10 10, 0 10, 0 0)), LINESTRING(5 0, 5 10), POINT(2 2))'));
	        SELECT count(*) FROM city.face WHERE face_id <> 0;
	        -- 2
	     )"});

	RegisterTopoFunction(loader, {"ST_AddIsoNode",
	                              {{varchar, integer, geometry}},
	                              {"st_addisonode"},
	                              {integer},
	                              ExecAddIsoNode,
	                              EDIT_NOTE,
	                              R"(
	        Adds an isolated node to a face of a topology and returns its id.

	        `ST_AddIsoNode(toponame, face, point)`: if `face` is NULL the face containing the point is computed, otherwise the point must lie in that face.

	        Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- invalid point`, `- coincident node` (a node already exists at that location), `- edge crosses node.` (the point lies on an edge), `- not within face` (the point is not in the given face), `Geometry SRID (...) does not match topology SRID (...)`.
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        SELECT * FROM ST_AddIsoNode('city', NULL, ST_Point(1, 1));
	        -- 1
	     )"});

	RegisterTopoFunction(loader, {"ST_AddIsoEdge",
	                              {{varchar, integer, integer, geometry}},
	                              {"st_addisoedge"},
	                              {integer},
	                              ExecAddIsoEdge,
	                              EDIT_NOTE,
	                              R"(
	        Adds an isolated edge between two isolated nodes of the same face and returns its id.

	        `ST_AddIsoEdge(toponame, start_node, end_node, line)`. Both nodes stop being isolated (`containing_face` becomes NULL).

	        Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- invalid curve`, `- curve not simple`, `- non-existent node`, `- not isolated node`, `- nodes in different faces`, `- start node not geometry start point.`, `- end node not geometry end point.`, `- geometry crosses a node`, `- geometry crosses edge N`, `- coincident edge N`, `Spatial exception - geometry intersects edge N`, and `Closed edges would not be isolated, try ST_AddEdgeNewFaces` when both nodes are the same.
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        SELECT * FROM ST_AddIsoNode('city', NULL, ST_Point(0, 0));
	        SELECT * FROM ST_AddIsoNode('city', NULL, ST_Point(5, 0));
	        SELECT * FROM ST_AddIsoEdge('city', 1, 2, ST_GeomFromText('LINESTRING(0 0, 5 0)'));
	        -- 1
	     )"});

	RegisterTopoFunction(loader, {"ST_AddEdgeNewFaces",
	                              {{varchar, integer, integer, geometry}},
	                              {"st_addedgenewfaces"},
	                              {integer},
	                              ExecAddEdge<false>,
	                              EDIT_NOTE,
	                              R"(
	        Adds an edge between two existing nodes and returns its id. If the edge splits a face, the face is deleted and replaced by two new faces.

	        `ST_AddEdgeNewFaces(toponame, start_node, end_node, line)`. The links of the adjacent edges are updated, as are the `left_face` / `right_face` of every edge and the `containing_face` of every isolated node of the split face. When the universal face is split only the bounded side gets a new face.

	        Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- invalid curve`, `- curve not simple`, `- non-existent node`, `- start node not geometry start point.`, `- end node not geometry end point.`, `- geometry crosses a node`, `- geometry crosses edge N`, `- coincident edge N`, `Spatial exception - geometry intersects edge N`, `Geometry SRID (...) does not match topology SRID (...)`.
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        SELECT * FROM ST_AddIsoNode('city', NULL, ST_Point(0, 0));
	        SELECT * FROM ST_AddEdgeNewFaces('city', 1, 1, ST_GeomFromText('LINESTRING(0 0, 10 0, 10 10, 0 10, 0 0)'));
	        SELECT face_id FROM city.face;
	        -- 0 and 1
	     )"});

	RegisterTopoFunction(loader, {"ST_AddEdgeModFace",
	                              {{varchar, integer, integer, geometry}},
	                              {"st_addedgemodface"},
	                              {integer},
	                              ExecAddEdge<true>,
	                              EDIT_NOTE,
	                              R"(
	        Adds an edge between two existing nodes and returns its id. If the edge splits a face, the face is kept for one side and a new face is added for the other.

	        `ST_AddEdgeModFace(toponame, start_node, end_node, line)`. The new face is created on the left of the new edge whenever that side is bounded, and on the right otherwise. The links of the adjacent edges are updated, as are the `left_face` / `right_face` of the edges, the `containing_face` of the isolated nodes that end up in the new face, and the bounding box of the modified face.

	        Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- invalid curve`, `- curve not simple`, `- non-existent node`, `- start node not geometry start point.`, `- end node not geometry end point.`, `- geometry crosses a node`, `- geometry crosses edge N`, `- coincident edge N`, `Spatial exception - geometry intersects edge N`, `Geometry SRID (...) does not match topology SRID (...)`.
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText(
	            'GEOMETRYCOLLECTION(POLYGON((0 0, 10 0, 10 10, 0 10, 0 0)), POINT(5 0), POINT(5 10))'));
	        SET VARIABLE a = (SELECT node_id FROM city.node WHERE ST_Equals(geom, ST_Point(5, 0)));
	        SET VARIABLE b = (SELECT node_id FROM city.node WHERE ST_Equals(geom, ST_Point(5, 10)));
	        SELECT * FROM ST_AddEdgeModFace('city', getvariable('a'), getvariable('b'),
	            ST_GeomFromText('LINESTRING(5 0, 5 10)'));
	        SELECT count(*) FROM city.face WHERE face_id <> 0;
	        -- 2
	     )"});

	RegisterTopoFunction(loader, {"ST_RemEdgeNewFace",
	                              {{varchar, integer}},
	                              {"st_remedgenewface"},
	                              {integer},
	                              ExecRemEdge<false>,
	                              EDIT_NOTE,
	                              R"(
	        Removes an edge. If it separates two faces, both are deleted and replaced by a new face covering them.

	        `ST_RemEdgeNewFace(toponame, edge)` returns the id of the new face, or NULL when no face is created: the edge had the same face on both sides, or one of its sides was the universal face (which then absorbs the other). End nodes left without edges become isolated nodes of the resulting face.

	        Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- non-existent edge N`.
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText(
	            'GEOMETRYCOLLECTION(POLYGON((0 0, 10 0, 10 10, 0 10, 0 0)), LINESTRING(5 0, 5 10))'));
	        SET VARIABLE middle = (SELECT edge_id FROM city.edge WHERE ST_Intersects(geom, ST_Point(5, 5)));
	        SELECT * FROM ST_RemEdgeNewFace('city', getvariable('middle'));
	        SELECT count(*) FROM city.face WHERE face_id <> 0;
	        -- 1
	     )"});

	RegisterTopoFunction(loader, {"ST_RemEdgeModFace",
	                              {{varchar, integer}},
	                              {"st_remedgemodface"},
	                              {integer},
	                              ExecRemEdge<true>,
	                              EDIT_NOTE,
	                              R"(
	        Removes an edge. If it separates two faces, one is deleted and the other is modified to cover both.

	        `ST_RemEdgeModFace(toponame, edge)` returns the id of the face that remains in place of the edge. The face on the right of the edge is kept, unless one side is the universal face, which then absorbs the other. End nodes left without edges become isolated nodes of that face.

	        Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- non-existent edge N`.
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText(
	            'GEOMETRYCOLLECTION(POLYGON((0 0, 10 0, 10 10, 0 10, 0 0)), LINESTRING(5 0, 5 10))'));
	        SET VARIABLE middle = (SELECT edge_id FROM city.edge WHERE ST_Intersects(geom, ST_Point(5, 5)));
	        SELECT * FROM ST_RemEdgeModFace('city', getvariable('middle'));
	        SELECT count(*) FROM city.face WHERE face_id <> 0;
	        -- 1
	     )"});

	RegisterTopoFunction(loader, {"ST_ChangeEdgeGeom",
	                              {{varchar, integer, geometry}},
	                              {"st_changeedgegeom"},
	                              {varchar},
	                              ExecChangeEdgeGeom,
	                              EDIT_NOTE,
	                              R"(
	        Changes the shape of an edge without changing the structure of the topology. Returns the text `Edge N changed`.

	        `ST_ChangeEdgeGeom(toponame, edge, line)`: the new line must keep the end points of the edge, must not meet any other edge or node, and must not sweep over a node or change the order of the edges around its end nodes. The bounding boxes of the faces on both sides are updated.

	        Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- invalid curve`, `- curve not simple`, `- non-existent edge N`, `- start node not geometry start point.`, `- end node not geometry end point.`, `- geometry crosses a node`, `- geometry crosses edge N`, `- coincident edge N`, `Spatial exception - geometry intersects edge N`, `Edge twist at node POINT(x y)`, `Edge motion collision at POINT(x y)`, `Edge changed disposition around start node N`, `Edge changed disposition around end node N`, `Edge ring changes winding`.
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText('LINESTRING(0 0, 10 0)'));
	        SELECT * FROM ST_ChangeEdgeGeom('city', 1, ST_GeomFromText('LINESTRING(0 0, 5 2, 10 0)'));
	        -- Edge 1 changed
	     )"});

	RegisterTopoFunction(loader, {"ST_ModEdgeSplit",
	                              {{varchar, integer, geometry}},
	                              {"st_modedgesplit"},
	                              {integer},
	                              ExecSplitEdge<true>,
	                              EDIT_NOTE,
	                              R"(
	        Splits an edge by creating a node on it, modifying the original edge and adding a new one. Returns the id of the new node.

	        `ST_ModEdgeSplit(toponame, edge, point)`: the original edge keeps its id and now ends at the new node; the new edge runs from the new node to the old end node.

	        Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- invalid point`, `- non-existent edge`, `- coincident node`, `- point not on edge`.
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText('LINESTRING(0 0, 10 0)'));
	        SELECT * FROM ST_ModEdgeSplit('city', 1, ST_Point(4, 0));
	        -- 3
	     )"});

	RegisterTopoFunction(loader, {"ST_NewEdgesSplit",
	                              {{varchar, integer, geometry}},
	                              {"st_newedgessplit"},
	                              {integer},
	                              ExecSplitEdge<false>,
	                              EDIT_NOTE,
	                              R"(
	        Splits an edge by creating a node on it, deleting the original edge and replacing it with two new edges. Returns the id of the new node.

	        `ST_NewEdgesSplit(toponame, edge, point)`: the first new edge runs from the old start node to the new node, the second from the new node to the old end node.

	        Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- invalid point`, `- non-existent edge`, `- coincident node`, `- point not on edge`.
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText('LINESTRING(0 0, 10 0)'));
	        SELECT * FROM ST_NewEdgesSplit('city', 1, ST_Point(4, 0));
	        SELECT edge_id FROM city.edge ORDER BY edge_id;
	        -- 2 and 3
	     )"});

	RegisterTopoFunction(loader, {"ST_ModEdgeHeal",
	                              {{varchar, integer, integer}},
	                              {"st_modedgeheal"},
	                              {integer},
	                              ExecHealEdges<true>,
	                              EDIT_NOTE,
	                              R"(
	        Heals two edges by deleting the node connecting them, modifying the first edge and deleting the second. Returns the id of the deleted node.

	        `ST_ModEdgeHeal(toponame, edge, other_edge)`: the first edge keeps its id and direction and takes over the geometry of both.

	        Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- non-existent edge N`, `- non-connected edges`, `- other edges connected (ids)` when the shared node has other edges, `Cannot heal edge N with itself, try with another`, `Edge N is closed, cannot heal to edge M`.
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText('GEOMETRYCOLLECTION(LINESTRING(0 0, 10 0), POINT(4 0))'));
	        SELECT * FROM ST_ModEdgeHeal('city', 1, 2);
	     )"});

	RegisterTopoFunction(loader, {"ST_NewEdgeHeal",
	                              {{varchar, integer, integer}},
	                              {"st_newedgeheal"},
	                              {integer},
	                              ExecHealEdges<false>,
	                              EDIT_NOTE,
	                              R"(
	        Heals two edges by deleting the node connecting them and replacing both edges with a new one, which has the direction of the first edge. Returns the id of the new edge.

	        Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- non-existent edge N`, `- non-connected edges`, `- other edges connected (ids)` when the shared node has other edges, `Cannot heal edge N with itself, try with another`, `Edge N is closed, cannot heal to edge M`.
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText('GEOMETRYCOLLECTION(LINESTRING(0 0, 10 0), POINT(4 0))'));
	        SELECT * FROM ST_NewEdgeHeal('city', 1, 2);
	        -- 3
	     )"});

	RegisterTopoFunction(loader, {"ST_MoveIsoNode",
	                              {{varchar, integer, geometry}},
	                              {"st_moveisonode"},
	                              {varchar},
	                              ExecMoveIsoNode,
	                              EDIT_NOTE,
	                              R"(
	        Moves an isolated node to another location within its face. Returns the text `Isolated Node N moved to location x,y`.

	        Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- invalid point`, `- non-existent node`, `- not isolated node`, `- coincident node`, `- edge crosses node.`, `Cannot move isolated node across faces`.
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        SELECT * FROM ST_AddIsoNode('city', NULL, ST_Point(1, 1));
	        SELECT * FROM ST_MoveIsoNode('city', 1, ST_Point(2, 3));
	        -- Isolated Node 1 moved to location 2,3
	     )"});

	RegisterTopoFunction(loader, {"ST_RemoveIsoNode",
	                              {{varchar, integer}},
	                              {"st_removeisonode"},
	                              {varchar},
	                              ExecRemoveIsoNode,
	                              EDIT_NOTE,
	                              R"(
	        Removes an isolated node. Returns the text `Isolated node N removed`.

	        Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- non-existent node`, `- not isolated node`.
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        SELECT * FROM ST_AddIsoNode('city', NULL, ST_Point(1, 1));
	        SELECT * FROM ST_RemoveIsoNode('city', 1);
	        -- Isolated node 1 removed
	     )"});

	RegisterTopoFunction(loader, {"ST_RemoveIsoEdge",
	                              {{varchar, integer}},
	                              {"st_removeisoedge"},
	                              {varchar},
	                              ExecRemoveIsoEdge,
	                              EDIT_NOTE,
	                              R"(
	        Removes an isolated edge. Its end nodes become isolated nodes of the face the edge was in. Returns the text `Isolated edge N removed`.

	        Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- non-existent edge`, `- not isolated edge` (the edge is closed, bounds a face, or shares a node with another edge).
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText('LINESTRING(0 0, 10 0)'));
	        SELECT * FROM ST_RemoveIsoEdge('city', 1);
	        -- Isolated edge 1 removed
	     )"});

	RegisterTopoFunction(loader, {"ST_GetFaceGeometry",
	                              {{varchar, integer}},
	                              {"st_getfacegeometry"},
	                              {geometry},
	                              ExecGetFaceGeometry,
	                              READ_NOTE,
	                              R"(
	        Returns the polygon of a face, built from the edges that have the face on exactly one side.

	        Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- universal face has no geometry`, `- non-existent face.`. The result is an untyped `GEOMETRY` even when the topology has a SRID.
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))'));
	        SELECT ST_AsText(st_getfacegeometry) FROM ST_GetFaceGeometry('city', 1);
	        -- POLYGON ((0 0, 0 10, 10 10, 10 0, 0 0))
	     )"});

	RegisterTopoFunction(loader, {"ST_GetFaceEdges",
	                              {{varchar, integer}},
	                              sequence_names,
	                              sequence_types,
	                              ExecGetFaceEdges,
	                              READ_NOTE,
	                              R"(
	        Returns the ordered set of signed edges bounding a face, as rows of `(sequence, edge)`.

	        Each ring is walked with the face on its left: an edge is positive when it is followed in its own direction, negative otherwise. The enumeration of a ring starts from its edge with the smallest identifier; the outer ring comes first, then the holes ordered by their smallest edge. Edges that have the face on both sides are not part of its boundary and are not returned. An unknown face yields no rows.

	        Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`.
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))'));
	        SELECT * FROM ST_GetFaceEdges('city', 1);
	     )"});

	const vector<vector<LogicalType>> point_lookup = {{varchar, geometry, real}};
	RegisterTopoFunction(loader, {"GetNodeByPoint",
	                              point_lookup,
	                              {"getnodebypoint"},
	                              {integer},
	                              ExecPointLookup<topology::GetNodeByPoint>,
	                              READ_NOTE,
	                              R"(
	        Returns the id of the node within `tolerance` of a point, or 0 if there is none.

	        `GetNodeByPoint(toponame, point, tolerance)`. Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- invalid point`, `Two or more nodes found`.
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        SELECT * FROM ST_AddIsoNode('city', NULL, ST_Point(1, 1));
	        SELECT * FROM GetNodeByPoint('city', ST_Point(1, 1.5), 1);
	        -- 1
	     )"});

	RegisterTopoFunction(loader, {"GetEdgeByPoint",
	                              point_lookup,
	                              {"getedgebypoint"},
	                              {integer},
	                              ExecPointLookup<topology::GetEdgeByPoint>,
	                              READ_NOTE,
	                              R"(
	        Returns the id of the edge within `tolerance` of a point, or 0 if there is none.

	        `GetEdgeByPoint(toponame, point, tolerance)`. Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- invalid point`, `Two or more edges found`.
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText('LINESTRING(0 0, 10 0)'));
	        SELECT * FROM GetEdgeByPoint('city', ST_Point(5, 0.5), 1);
	        -- 1
	     )"});

	RegisterTopoFunction(loader, {"GetFaceByPoint",
	                              point_lookup,
	                              {"getfacebypoint"},
	                              {integer},
	                              ExecPointLookup<topology::GetFaceByPoint>,
	                              READ_NOTE,
	                              R"(
	        Returns the id of the face containing a point, or 0 if the point is in the universal face.

	        `GetFaceByPoint(toponame, point, tolerance)`: a point strictly inside a face yields that face whatever the tolerance. Otherwise the faces bounded by the edges within `tolerance` of the point are considered; a point in the universal face close to the boundary of a single face yields that face.

	        Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- invalid point`, `Two or more faces found` (in particular for a point lying on an edge shared by two faces).
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))'));
	        SELECT * FROM GetFaceByPoint('city', ST_Point(5, 5), 0);
	        -- 1
	     )"});

	RegisterTopoFunction(loader, {"GetNodeEdges",
	                              {{varchar, integer}},
	                              sequence_names,
	                              sequence_types,
	                              ExecGetNodeEdges,
	                              READ_NOTE,
	                              R"(
	        Returns the edges incident to a node, as rows of `(sequence, edge)` ordered clockwise starting from north.

	        An edge is positive when it starts at the node and negative when it ends there; a closed edge is reported twice. Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`.
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText('MULTILINESTRING((0 0, 10 0), (0 0, 0 10))'));
	        SELECT * FROM GetNodeEdges('city', 1);
	     )"});

	RegisterTopoFunction(loader, {"GetRingEdges",
	                              {{varchar, integer}, {varchar, integer, integer}},
	                              sequence_names,
	                              sequence_types,
	                              ExecGetRingEdges,
	                              READ_NOTE,
	                              R"(
	        Returns the ordered set of signed edges met by walking along one side of an edge, as rows of `(sequence, edge)`.

	        `GetRingEdges(toponame, edge, max_edges := NULL)`: a positive `edge` starts the walk on the left side of the edge in its own direction, a negative one on the right side in the opposite direction. The walk follows `next_left_edge` after a positive edge and `next_right_edge` after a negative one until it is back at the start. An unknown edge yields no rows.

	        Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `Max traversing limit hit: N` when the ring has more than `max_edges` edges.
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))'));
	        SELECT * FROM GetRingEdges('city', 1);
	     )"});

	RegisterTopoFunction(loader, {"ValidateTopology",
	                              {{varchar}},
	                              {"error", "id1", "id2"},
	                              {varchar, integer, integer},
	                              ExecValidateTopology,
	                              READ_NOTE,
	                              R"(
	        Checks a topology and returns one `(error, id1, id2)` row per problem found; a valid topology yields no rows.

	        | error | id1 | id2 |
	        | --- | --- | --- |
	        | coincident nodes | node | node |
	        | edge crosses node | edge | node |
	        | invalid edge | edge | |
	        | edge not simple | edge | |
	        | edge crosses edge | edge | edge |
	        | edge start node geometry mismatch | edge | node |
	        | edge end node geometry mismatch | edge | node |
	        | face without edges | face | |
	        | invalid next_right_edge | edge | expected value |
	        | invalid next_left_edge | edge | expected value |
	        | mixed face labeling in ring | signed edge of the ring | |
	        | universal face has shell rings | 0 | signed edge of the ring |
	        | face has multiple shells | face | signed edge of the ring |
	        | face has no rings | face | |
	        | face has wrong mbr | face | |
	        | hole not in advertised face | signed edge of the ring | |
	        | not-isolated node has not-null containing_face | node | |
	        | isolated node has null containing_face | node | |
	        | isolated node has wrong containing_face | node | |
	        | face within face | inner face | outer face |
	        | face overlaps face | face | face |

	        The expected links and rings are derived from the geometry of the edges. The ring, face and isolated-node checks are skipped when one of the first seven kinds of error is reported, since they need sound linework.

	        Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`. Unlike PostGIS there is no bounding-box argument: the whole topology is loaded and checked.
	     )",
	                              R"(
	        CALL CreateTopology('city');
	        SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))'));
	        SELECT * FROM ValidateTopology('city');
	        -- no rows
	     )"});

	RegisterTopoElementArrayAgg(loader);
}

} // namespace duckdb
