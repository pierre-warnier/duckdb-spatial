#include "spatial/modules/routing/routing_module.hpp"
#include "spatial/modules/routing/routing_common.hpp"

namespace duckdb {

void RegisterRoutingModule(ExtensionLoader &loader) {
	routing::RegisterRoutingOptimizer(loader);
	routing::RegisterPathFunctions(loader);
	routing::RegisterPointFunctions(loader);
	routing::RegisterFlowFunctions(loader);
	routing::RegisterTourFunctions(loader);
	routing::RegisterComponentFunctions(loader);
	routing::RegisterTopologyFunctions(loader);
}

} // namespace duckdb
