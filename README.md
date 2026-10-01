# DuckDB Spatial Extension (Enhanced Fork)

This fork of [duckdb/duckdb-spatial](https://github.com/duckdb/duckdb-spatial) extends the DuckDB spatial extension with **220 additional functions**, a **native KNN spatial join operator**, a **GEOG type**, a **RASTER type**, **PostGIS-style topologies**, **pgRouting-style network routing**, **DBSCAN/K-means clustering**, and significant **performance optimizations** to the spatial join pipeline. The goal is PostGIS parity and SedonaDB-competitive performance within DuckDB's analytical engine.

**385 documented functions** (vs. 165 upstream) | **225 tests / 12515 assertions** | Synced with upstream v1.5-variegata

**Table of contents**
- [What's new in this fork](#whats-new-in-this-fork)
- [What is this?](#what-is-this)
- [How do I get it?](#how-do-i-get-it)
- [Example Usage](#example-usage)
- [Supported Functions](#supported-functions-and-documentation)
- [Internals and Technical Details](#internals-and-technical-details)
- [Benchmarks against PostGIS](docs/benchmarks.md)

# What's new in this fork

## KNN Spatial Join

Native k-nearest-neighbor spatial join via `ST_KNN`, using Hjaltason-Samet priority-queue traversal over a FlatRTree. Candidates are refined with the exact geometry distance, so the result is the exact set of k nearest rows. Supports INNER and LEFT joins, and an optional partition key to get the k nearest rows of every group in a single join.

```sql
-- Find 5 nearest hydrants for each building
SELECT b.id, h.id, ST_Distance(b.geom, h.geom) AS dist
FROM buildings b
JOIN hydrants h ON ST_KNN(b.geom, h.geom, 5);

-- Find the nearest point of interest of every category for each building
SELECT b.id, p.category, ST_Distance(b.geom, p.geom) AS dist
FROM buildings b
JOIN pois p ON ST_KNN(b.geom, p.geom, 1, p.category);
```

Distances are planar, in the units of the coordinates: project longitude/latitude data to a metric CRS first. The searched side (the second geometry) is indexed in memory. See [`ST_KNN`](docs/functions.md#st_knn) in the function reference for the full semantics.

## Geography

A `GEOG` type for longitude/latitude data on the WGS84 ellipsoid: edges are geodesics, polygons include their interior, and results are in meters and square meters.

```sql
SELECT ST_Distance(ST_GeogPoint(4.3517, 50.8503), ST_GeogPoint(-74.006, 40.7128));   -- 5904542.0 m
SELECT ST_Area('POLYGON((4 50, 5 50, 5 51, 4 51, 4 50))'::GEOG);                -- 7892061583 m²
SELECT ST_Buffer(geog, 500), ST_DWithin(geog, ST_GeogPoint(4.3517, 50.8503), 10000) FROM places;
```

- Constructors: `ST_GeogPoint(lon, lat)`, `ST_GeogFromText` / `ST_GeogFromWKT`, `ST_GeogFromWKB`, and explicit casts from `VARCHAR` and `GEOMETRY`. Coordinates are always longitude then latitude, whatever `geometry_always_xy` says, and out-of-range values are rejected. A geometry that carries a CRS has to drop it first (`geom::GEOMETRY::GEOG`): nothing is reprojected.
- Geodesic overloads: `ST_Area`, `ST_Length`, `ST_Perimeter`, `ST_Distance`, `ST_DWithin`, `ST_Intersects`, `ST_Buffer`, `ST_Azimuth`, `ST_Project`, `ST_Segmentize`, `ST_AsText`, `ST_AsWKB`. Distances between edges are computed on the ellipsoid itself (checked against an independent implementation to a few nanometers), handle polygons around a pole or across the date line, and cost the product of the vertex counts in the worst case. `ST_Buffer` works in an azimuthal equidistant projection centered on the geography, so its accuracy decreases for geographies spanning hundreds of kilometers.
- The type is named `GEOG`, not `GEOGRAPHY`: DuckDB plans a `GEOGRAPHY` type of its own for v2.0, and this one must not collide with it.
- There is deliberately no implicit cast to `GEOMETRY`: the planar functions do not silently apply to geographies. Cast explicitly (`geog::GEOMETRY`) to use them.
- An untyped string literal or `NULL` still resolves to the `GEOMETRY` overload of these functions, so existing calls bind as before; next to a geography argument, a string literal is read as a geography.
- Joins on geography predicates run as regular joins, and R-tree indexes cannot be created on geography columns.
- A geography is stored as WKB under its own type name, so the column keeps its type in databases of any storage version. Other formats see a plain `BLOB`: cast to `GEOMETRY` before exporting to GeoParquet or through GDAL, and cast the WKB back with `::GEOG` when reading.

## Raster

A `RASTER` type and 94 PostGIS-style raster functions on top of the bundled GDAL.

```sql
-- Read a GeoTIFF as 256x256 tiles, then query it
CREATE TABLE dem AS SELECT * FROM ST_ReadRaster('dem.tif', 256, 256);
SELECT ST_Value(rast, ST_Point(152000, 167000)) FROM dem WHERE ST_Intersects(rast, ST_Point(152000, 167000));
SELECT ST_SummaryStats(ST_Slope(rast)) FROM dem;
SELECT ST_MapAlgebra(rast, 1, '32BF', '[rast] * 0.3048') FROM dem;   -- any constant DuckDB expression
SELECT ST_Union_Agg(rast) FROM dem;
```

- I/O (`ST_ReadRaster`, `ST_FromGDALRaster`, `ST_AsGDALRaster`, `ST_AsTIFF`), constructors and band management, accessors, pixel access and editing, `ST_Clip`, resampling and `ST_Transform`, terrain (`ST_Slope`, `ST_Aspect`, `ST_Hillshade`, `ST_TPI`, `ST_TRI`, `ST_Roughness`), statistics, `ST_Reclass`, `ST_ColorMap`, `ST_MapAlgebra` (one and two rasters), raster/vector conversion (`ST_AsRaster`, `ST_DumpAsPolygons`, `ST_Polygon`, `ST_Intersection`, `ST_Contour`), predicates, and the aggregates `ST_Union_Agg`, `ST_SummaryStatsAgg` and `ST_Retile`. Each function documents its differences from PostGIS in the [function reference](docs/functions.md).
- A raster value is an uncompressed GeoTIFF stored as a BLOB under the `RASTER` type name: about 17 µs to open a 256x256 tile, 0.3 ms to read all of its pixels. All the bands of a raster share one pixel type.
- The bundled GDAL only has the GeoTIFF, COG, HFA, VRT and MEM raster drivers: there is no PNG or JPEG.
- Set-returning PostGIS functions return lists (use `unnest`), the raster union is `ST_Union_Agg` rather than `ST_Union`, and `ST_Retile` is an aggregate. The callback forms of `ST_MapAlgebra` and the array-argument variants (band lists, `reclassarg[]`) are not available. Terrain functions use GDAL's handling of border pixels, which differs from PostGIS's on the outermost row and column.

## Topology

The PostGIS / ISO SQL-MM topology model: a topology is a schema holding `node`, `edge_data` (and the `edge` view) and `face` tables, registered in `topology.topology`, in which shared boundaries are stored once.

```sql
CALL CreateTopology('parcels', 31370);
SET VARIABLE linework = (SELECT ST_Collect(list(geom)) FROM parcels);
SELECT * FROM ST_CreateTopoGeo('parcels', getvariable('linework'));
SELECT * FROM ST_GetFaceGeometry('parcels', 1);
SELECT * FROM ValidateTopology('parcels');
```

- 30 functions: management (`CreateTopology`, `DropTopology`, `GetTopologyID`, `GetTopologySRID`, `GetTopologyName`, `TopologySummary`), population (`ST_CreateTopoGeo`), ISO editing (`ST_AddIsoNode`, `ST_AddIsoEdge`, `ST_AddEdgeNewFaces`, `ST_AddEdgeModFace`, `ST_RemEdgeNewFace`, `ST_RemEdgeModFace`, `ST_ChangeEdgeGeom`, `ST_ModEdgeSplit`, `ST_NewEdgesSplit`, `ST_ModEdgeHeal`, `ST_NewEdgeHeal`, `ST_MoveIsoNode`, `ST_RemoveIsoNode`, `ST_RemoveIsoEdge`), accessors (`ST_GetFaceGeometry`, `ST_GetFaceEdges`, `GetNodeByPoint`, `GetEdgeByPoint`, `GetFaceByPoint`, `GetNodeEdges`, `GetRingEdges`), `ValidateTopology` and the `TopoElementArray_Agg` aggregate. The TopoGeometry layer (`CreateTopoGeom`, `toTopoGeom`, ...) is not implemented.
- Every function except the aggregate is a table function: call it with `CALL f(...)` or `SELECT * FROM f(...)`, not as a scalar. Its arguments must be constants; to pass a value computed by a query, store it first with `SET VARIABLE v = (SELECT ...)` and pass `getvariable('v')`.
- An edit runs in its own transaction on a separate connection and is committed when the call returns: a later `ROLLBACK` of the caller does not undo it, and a failed edit changes nothing. An edit is refused while the caller has uncommitted changes in an explicit transaction.
- Topologies are two-dimensional, the topology tables have no spatial index (the cost of an edit grows with the size of the topology), and the functions need GEOS.

## Network routing

pgRouting's functions over any edge table, passed as a table-valued argument and matched by column name (`id, source, target, cost [, reverse_cost, capacity, x1, y1, x2, y2]`). A negative cost means the edge cannot be used in that direction.

```sql
SELECT * FROM pgr_dijkstra((SELECT id, source, target, cost, reverse_cost FROM edges), 1, 5);
SELECT * FROM pgr_dijkstra(TABLE edges, [1, 2], [5, 6], directed := false);
SELECT * FROM pgr_drivingDistance((SELECT * FROM edges), 1, 600);
SELECT * FROM pgr_TSP((SELECT * FROM pgr_dijkstraCostMatrix((SELECT * FROM edges), [1, 2, 3, 4])), 1);
```

- Shortest paths: `pgr_dijkstra`, `pgr_aStar`, `pgr_bdDijkstra`, `pgr_bdAstar`, `pgr_dijkstraCost`, `pgr_dijkstraCostMatrix`, `pgr_KSP`, `pgr_drivingDistance`, `pgr_withPoints`, `pgr_trsp`; `pgr_TSP`; flows `pgr_maxFlow`, `pgr_maxFlowMinCost` / `pgr_minCostMaxFlow`, `pgr_pushRelabel`, `pgr_edmondsKarp`, `pgr_boykovKolmogorov`; `pgr_connectedComponents`, `pgr_strongComponents`; and the topology helpers `pgr_extractVertices`, `pgr_createTopology`, `pgr_nodeNetwork`, `pgr_analyzeGraph`, which return rows instead of altering tables. Results match pgRouting's documentation on its sample data.
- The edges arrive through a single table argument, inside the caller's transaction, so CTEs, temp tables and uncommitted rows work. A second table cannot be passed, so points of interest, turn restrictions and the TSP matrix are given as a list of structs, typically `SET VARIABLE points = (SELECT list(p) FROM pois p)` then `getvariable('points')`.
- About 1 s per call on a 2M-edge grid, almost all of it spent loading the graph; the search itself is a binary-heap Dijkstra over a CSR adjacency.
- Not implemented: the "combinations" signatures; equal-cost ties may choose a different path than pgRouting. Road routing with turn costs, traffic and matrices at scale remains the job of a dedicated routing engine.

## Spatial Clustering

PostGIS-compatible window functions for density-based and partition-based clustering.

```sql
-- DBSCAN: find clusters with eps=100m, minpoints=5
SELECT *, ST_ClusterDBSCAN(geom, 100.0, 5) OVER (ORDER BY id) AS cluster_id
FROM retail_points;

-- K-means: partition into 10 clusters
SELECT *, ST_ClusterKMeans(geom, 10) OVER (ORDER BY id) AS cluster_id
FROM sensor_locations;
```

Also includes `ST_ClusterIntersecting` and `ST_ClusterWithin` aggregate functions.

## Performance

Measured against PostGIS 3.5 / pgRouting 4.0 on the same machine and the same data, this fork is faster on all 22 operations of the benchmark, on one core as well as on all cores, with identical results: see [docs/benchmarks.md](docs/benchmarks.md) for the queries, the numbers and the method.

- **Point-in-polygon joins**: `ST_Intersects`, `ST_Contains`, `ST_Within`, `ST_Covers` and `ST_CoveredBy` answer point-versus-polygon with an exact ray crossing count on the serialized polygon, without GEOS, and the spatial join keeps an index of the build-side polygons it tests (about 10x faster on one thread)
- **Parallel overlays**: `ST_Intersection`, `ST_Difference` and `ST_Union` compute the rows of a chunk as parallel tasks when the geometries are large, so a few thousand big polygons use all the cores instead of one
- **Spatial join pipeline**: envelope pre-check before R-tree descent, BFS-to-DFS traversal (better cache locality), Hilbert sort permutation for sequential row access (~1.7x measured), batch bbox extraction
- **R-tree STR bulk loading**: full Sort-Tile-Recursive packing for the persistent R-tree index, improving query-time fan-out
- **Hot-path cleanups**: `pow(x,2)` replaced with `x*x` across all distance kernels, `std::sort` replaces hand-rolled quicksort in FlatRTree
- **Robust predicates**: Shewchuk adaptive-precision `orient2d` for exact point-in-polygon and intersection tests near collinear edges

## 220 New Functions (PostGIS parity)

| Category | Functions |
|---|---|
| **Serialization** (16) | `ST_AsGML`, `ST_GeomFromGML`, `ST_AsKML`, `ST_GeomFromKML`, `ST_AsEWKB`, `ST_AsEWKT`, `ST_AsTWKB`, `ST_GeomFromEWKB`, `ST_GeomFromEWKT`, `ST_GeomFromTWKB`, `ST_AsEncodedPolyline`, `ST_LineFromEncodedPolyline`, `ST_GeoHash`, `ST_GeomFromGeoHash`, `ST_Box2dFromGeoHash`, `ST_AsLatLonText` |
| **GEOS Construction** (13) | `ST_ClipByBox2D`, `ST_DelaunayTriangles`, `ST_GeometricMedian`, `ST_LargestEmptyCircle`, `ST_MinimumBoundingCircle`, `ST_MinimumClearance`, `ST_MinimumClearanceLine`, `ST_OffsetCurve`, `ST_SharedPaths`, `ST_SimplifyPolygonHull`, `ST_Split`, `ST_TriangulatePolygon`, `ST_UnaryUnion` |
| **Geometry Editing** (15) | `ST_AddPoint`, `ST_SetPoint`, `ST_RemovePoint`, `ST_ChaikinSmoothing`, `ST_ForceCollection`, `ST_QuantizeCoordinates`, `ST_Scroll`, `ST_Segmentize`, `ST_SetSRID`, `ST_ShiftLongitude`, `ST_SimplifyVW`, `ST_SwapOrdinates`, `ST_ForcePolygonCCW`, `ST_ForcePolygonCW`, `ST_SnapToGrid` |
| **Accessors** (11) | `ST_BoundingDiagonal`, `ST_GeometryN`, `ST_IsCollection`, `ST_IsPolygonCCW`, `ST_IsPolygonCW`, `ST_IsValidDetail`, `ST_IsValidReason`, `ST_MemSize`, `ST_NRings`, `ST_SRID`, `ST_Summary` |
| **3D / Measure** (7) | `ST_3DDistance`, `ST_3DLength`, `ST_3DLineInterpolatePoint`, `ST_3DPerimeter`, `ST_AddMeasure`, `ST_CoordDim`, `ST_NDims` |
| **Distance / Proximity** (5) | `ST_Angle`, `ST_FrechetDistance`, `ST_HausdorffDistance`, `ST_LongestLine`, `ST_MaxDistance` |
| **Clustering** (4) | `ST_ClusterDBSCAN`, `ST_ClusterIntersecting`, `ST_ClusterKMeans`, `ST_ClusterWithin` |
| **Predicates** (4) | `ST_DFullyWithin`, `ST_OrderingEquals`, `ST_Relate`, `ST_RelateMatch` |
| **Decomposition** (3) | `ST_DumpPoints`, `ST_DumpRings`, `ST_DumpSegments` |
| **Constructors** (2) | `ST_LineFromMultiPoint`, `ST_Polygon` |
| **Grids** (2) | `ST_HexagonGrid`, `ST_SquareGrid` |
| **Routing** (23) | see [Network routing](#network-routing) |
| **Raster** (78 names, 94 functions with overloads of existing names) | see [Raster](#raster) |
| **Topology** (30) | see [Topology](#topology) |
| **Geography** (5) | `ST_GeogPoint`, `ST_GeogFromText`, `ST_GeogFromWKT`, `ST_GeographyFromText`, `ST_GeogFromWKB` |
| **Geodesic** (1) | `ST_Project` |
| **Join** (1) | `ST_KNN` |

## Backward Compatibility

Databases written by duckdb-spatial before DuckDB v1.5 (when GEOMETRY was a BLOB alias) are automatically readable. The extension registers an implicit cast that walks the legacy binary format and reserializes to the native GEOMETRY layout.

## Correctness Fixes

- GEOS deserialization alignment fix (double-aligned buffers for `GEOSCoordSeq_copyFromBuffer_r`)
- `ST_ForceCollection` deep copy for nested multi-part geometries
- `ST_3DDistance` / `ST_DFullyWithin` restricted to POINT inputs (vertex-only computation is incorrect for lines/polygons)
- `ST_Intersects` fallback uses exact distance instead of bbox-only heuristic
- `robust::init()` thread safety via `std::call_once`
- Spatial join dirty validity mask fix (cherry-picked from upstream #812), and the same fix for the KNN join, which silently dropped probe rows that followed a chunk containing NULL or empty geometries
- `ST_ClusterDBSCAN` / `ST_ClusterKMeans` assigned cluster ids to the wrong rows when a partition spanned several chunks on several threads; cluster ids now follow their rows, and `PARTITION BY` works without `ORDER BY`
- `ST_GeometryN` counts from 1 and returns NULL out of range, as in PostGIS (it counted from 0 and raised an error)
- Scalar functions return a constant vector for constant arguments (an assertion failure in debug builds of DuckDB), and `ST_AsEncodedPolyline` no longer shifts negative values

---

# What is this?

This is a geospatial extension for DuckDB that adds support for working with spatial data and functions in the form of a `GEOMETRY` type based on the "Simple Features" geometry model, as well as non-standard specialized columnar DuckDB native geometry types that provide better compression and faster execution in exchange for flexibility.

See the [function table](docs/functions.md) for the current implementation status.

# How do I get it?

## Building from source

```bash
git clone --recurse-submodules https://github.com/pierre-warnier/duckdb-spatial
cd duckdb-spatial
GEN=ninja make release
```

You can then invoke the built DuckDB (with the extension statically linked):

```bash
./build/release/duckdb
```

**Dependencies**: CMake 3.20+, a C++17 compiler, OpenSSL (`sudo apt install libssl-dev` on Ubuntu), and [Ninja](https://ninja-build.org) (recommended). All other dependencies are bundled.

## Using the build from another DuckDB client

The build also produces a loadable extension, laid out as a local extension repository in `build/release/repository`. It can only be loaded by a DuckDB of the exact version it was built against (currently v1.5.6), on the same platform.

```bash
make install-local
```

installs it into the extension directory of the current user (`~/.duckdb/extensions`), replacing the official `spatial` extension for that DuckDB version. It is equivalent to running, from any client:

```sql
FORCE INSTALL spatial FROM '/path/to/duckdb-spatial/build/release/repository';
```

After that, a plain `INSTALL spatial; LOAD spatial;` keeps working in every client, and `duckdb_extensions()` reports `install_mode = REPOSITORY` with the commit of this repository as `extension_version`. `FORCE INSTALL spatial FROM core;` goes back to the official extension.

A local build is not signed, so the connection that loads it has to be opened with unsigned extensions allowed. This cannot be changed once the database is open:

| Client | Setting |
|---|---|
| CLI | `duckdb -unsigned` |
| Python | `duckdb.connect(config={'allow_unsigned_extensions': 'true'})` |
| C API | `duckdb_set_config(config, "allow_unsigned_extensions", "true")` before `duckdb_open_ext` |
| Rust | `Connection::open_with_flags(path, Config::default().allow_unsigned_extensions()?)` |

Do not copy `spatial.duckdb_extension` over an installed one by hand: the `.info` file next to it still describes the previous binary, and loading by name then fails with `Metadata mismatch detected when loading extension`. Loading by explicit path (`LOAD '/path/to/spatial.duckdb_extension'`) does work.

# Example Usage

See the [example](docs/example.md) for basic usage.

# Supported Functions and Documentation

The full list of functions and their documentation is available in the [function reference](docs/functions.md).

# Internals and Technical Details

See the [internals documentation](docs/internals.md) for details on the internal workings of the extension.
