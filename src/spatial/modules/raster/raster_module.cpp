#include "spatial/modules/raster/raster_module.hpp"
#include "spatial/modules/raster/raster_core.hpp"

#include "gdal_frmts.h"

#include <mutex>

namespace duckdb {

void RegisterRasterModule(ExtensionLoader &loader) {
	static std::once_flag loaded;
	std::call_once(loaded, []() {
		GDALRegister_GTiff();
		GDALRegister_COG();
		GDALRegister_MEM();
		GDALRegister_VRT();
		GDALRegister_HFA();
		GDALRegister_Derived();
		// RASTER values live in /vsimem: never write .aux.xml side-car files next to them
		CPLSetConfigOption("GDAL_PAM_ENABLED", "NO");
	});

	raster::RegisterRasterType(loader);
	raster::RegisterRasterBasicFunctions(loader);
	raster::RegisterRasterIOFunctions(loader);
	raster::RegisterRasterProcessingFunctions(loader);
	raster::RegisterRasterStatisticsFunctions(loader);
}

} // namespace duckdb
