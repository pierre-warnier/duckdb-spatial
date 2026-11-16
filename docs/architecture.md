# Architecture of the fork

Where the code lives and how the main pieces fit together.


## Load sequence

`src/spatial/spatial_extension.cpp` is the single entry point. `LoadInternal` registers, in order: types (`GeoTypes`), settings, the core function families (cast, scalar, aggregate, table, window), the spatial join optimizer, then each module (PROJ, GDAL, GEOS, OSM, shapefile, MVT, WKB), then the R-tree index, then the operator extension. A new module or function family is wired in here.

## Geometry representation

- `GEOMETRY` is DuckDB core's native type as of v1.5 (`LogicalType::GEOMETRY()`), not an extension-defined blob alias. Pre-1.5 databases stored it as `BLOB` aliased `GEOMETRY`; `spatial_functions_cast.cpp` registers an implicit cast that rewrites the legacy layout.
- `GeoTypes` (`spatial_types.cpp`) defines the columnar alias types built on `STRUCT`/`LIST`: `POINT_2D/3D/4D`, `LINESTRING_2D/3D`, `POLYGON_2D/3D`, `BOX_2D`, `BOX_2DF`, plus `WKB_BLOB`. Many functions have a separate overload per type.
- **SGL** (`src/sgl/`) is the dependency-free in-memory geometry library (`sgl::geometry`, `sgl::ops::*`, prepared geometries, Shewchuk robust predicates in `robust_predicates.cpp`). It is built as part of the extension and knows nothing about DuckDB.
- `src/spatial/geometry/geometry_serialization.*` (`Serde`) converts between the stored blob and `sgl::geometry`. Deserialized geometries are arena-allocated; nothing is individually freed.

## How a function is implemented

Nearly all native functions live in one file per kind under `src/spatial/modules/main/` (`spatial_functions_scalar.cpp` is ~12k lines). The pattern:

- One `struct ST_Foo` per SQL function, holding static `Execute*` functions (one per overload) and a static `Register(ExtensionLoader &)`.
- `Register` uses `FunctionBuilder::RegisterScalar` / `RegisterAggregate` / `RegisterMacro` (`src/spatial/util/function_builder.hpp`): `AddVariant` per overload with `AddParameter` / `SetReturnType` / `SetFunction`, then `SetDescription`, `SetExample`, `SetTag("ext", "spatial")` and a `category` tag. The description and tags feed the generated docs; a function without the `ext` tag is left out of `docs/functions.md`.
- Variants that touch `GEOMETRY` set `SetInit(LocalState::Init)` and call `LocalState::ResetAndGet(state)` at the top of `Execute`. The local state owns a per-thread `ArenaAllocator` that is reset once per chunk; `lstate.Deserialize` / `lstate.Serialize` go through it.
- Functions returning geometry set `SetBind(GeoTypes::PropagateCRS)` so the CRS of the input type carries to the result type.
- The struct's `Register` must be called from the matching `RegisterSpatial*Functions` list at the bottom of the file.

Functions that need GEOS follow the same builder pattern in `modules/geos/geos_module.cpp`, with `geos_serde.*` converting directly between the stored blob and GEOS handles. GEOS is optional (`SPATIAL_USE_GEOS`), so anything in `modules/main` must not depend on it. Prefer a native SGL implementation and fall back to GEOS only for operations SGL lacks.

## Spatial joins

There is no SQL syntax for the join operators; they are injected by an optimizer extension.

1. `operators/spatial_join_optimizer.cpp` walks the logical plan and rewrites a `LogicalAnyJoin` / comparison join whose condition is a recognised predicate into a `LogicalSpatialJoin`. The set of recognised predicates is the `spatial_predicate_map` table in that file, with `spatial_predicate_inverse_map` used to swap sides. A new predicate that implies bbox intersection has to be added to both tables to get the join.
2. `ST_DWithin` is handled through `ST_DWithinHelper::TryGetConstDistance` (`util/distance_extract.hpp`), which pulls the constant distance out of the function's bind data to inflate the probe bbox.
3. `ST_KNN(a, b, k [, partition])` is a marker function: executing it throws ("cannot be used outside of a JOIN ON clause"). The optimizer recognises it, reads `k` via `ST_KNNHelper::TryGetConstK` (`util/knn_extract.hpp`), and produces a `LogicalSpatialKNNJoin`.
4. The physical operators (`spatial_join_physical.cpp`, `spatial_knn_join_physical.cpp`) materialise the build side, bulk-load an in-memory `FlatRTree` (`operators/flat_rtree.hpp`, Hilbert-sorted), and probe it per input chunk. The KNN operator does a priority-queue traversal with over-fetch and exact distance refinement.
5. `spatial_operator_extension.cpp` registers an `OperatorExtension` whose `Deserialize` dispatches on `OPERATOR_TYPE_NAME`. Every custom logical operator (`LogicalSpatialJoin`, `LogicalSpatialKNNJoin`, `LogicalCreateRTreeIndex`) must be listed there or plan serialization fails.

`FlatRTree` (transient, join-only) and the persistent R-tree index are separate implementations.

A point-versus-polygon predicate is answered without GEOS by `util/point_in_area.hpp`. Around the evaluation of its predicate the join makes a `PreparedAreaCache` current (thread-local), which indexes build-side polygons by the address of their serialized form; this is only valid because build rows do not move while the join runs, and only for the argument the join passes its build side as.

## Persistent R-tree index

`src/spatial/index/rtree/` implements `CREATE INDEX ... USING RTREE` as a DuckDB `BoundIndex`:

- `rtree.cpp` / `rtree_node.hpp` — the tree, stored in DuckDB's fixed-size block allocator
- `rtree_index_create_logical.cpp` / `_physical.cpp` — index build, with STR bulk loading
- `rtree_index_plan_scan.cpp` — optimizer rule that replaces a sequential scan + spatial filter with `rtree_index_scan`
- `rtree_index_pragmas.cpp` — `rtree_index_dump` and info pragmas

## I/O modules

Each directory under `src/spatial/modules/` is self-contained and exposes one `Register*Module` function. `gdal` provides `ST_Read` and `COPY ... (FORMAT GDAL)` and bridges GDAL's VSI to DuckDB's file system; `proj` embeds `proj.db` in the binary (`proj_db.c`, served through the in-memory SQLite VFS in `memvfs.c`); `shapefile`, `osm`, `mvt` and `wkb` are native readers/writers that do not go through GDAL.

## Fork-specific surface

Relative to upstream, this fork adds the `GEOG` type with geodesic operations (`modules/geography`, a named BLOB holding WKB, no implicit cast to GEOMETRY), a `RASTER` type and raster functions on GDAL (`modules/raster`, a named BLOB holding a GeoTIFF; it must be registered after every module whose aggregates it overloads, and `test/sql/raster/generate_examples_test.py` regenerates the test that runs the documentation examples), pgRouting-style network routing (`modules/routing`, table in-out functions taking the edges as a TABLE argument; an optimizer extension puts an ORDER BY under the function so the whole input arrives through one pipeline), PostGIS-style topologies (`modules/topology`, table functions that edit the topology tables through a separate connection in their own transaction), the KNN join operator, the clustering window/aggregate functions (`spatial_functions_window.cpp`, `ST_ClusterDBSCAN` / `ST_ClusterKMeans` / `ST_ClusterIntersecting` / `ST_ClusterWithin`), robust predicates in SGL, STR bulk loading for the R-tree index, GML/KML conversion, and the other PostGIS-parity functions listed in `README.md`. Upstream merges conflict most often in `spatial_functions_scalar.cpp`, `geos_module.cpp`, `spatial_join_optimizer.cpp` and `spatial_join_physical.cpp`.
