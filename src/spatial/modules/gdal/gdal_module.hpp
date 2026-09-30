#pragma once
#include "duckdb/main/extension/extension_loader.hpp"

namespace duckdb {

class ExtensionLoader;
class ClientContext;

void RegisterGDALModule(ExtensionLoader &loader);
// Path that GDAL opens through DuckDB's file system for this client
string GDALFileSystemPath(ClientContext &context, const string &path);
void RegisterExtraFunction(ExtensionLoader &loader);
} // namespace duckdb
