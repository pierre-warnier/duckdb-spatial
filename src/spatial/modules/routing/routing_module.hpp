#pragma once

namespace duckdb {

class ExtensionLoader;

void RegisterRoutingModule(ExtensionLoader &loader);

namespace routing {

void RegisterPathFunctions(ExtensionLoader &loader);
void RegisterPointFunctions(ExtensionLoader &loader);
void RegisterFlowFunctions(ExtensionLoader &loader);
void RegisterTourFunctions(ExtensionLoader &loader);
void RegisterComponentFunctions(ExtensionLoader &loader);
void RegisterTopologyFunctions(ExtensionLoader &loader);

} // namespace routing

} // namespace duckdb
