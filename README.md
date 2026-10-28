# DuckDB Spatial Extension (Enhanced Fork)

This fork of [duckdb/duckdb-spatial](https://github.com/duckdb/duckdb-spatial) extends the DuckDB spatial extension with **213 additional functions**, a **native KNN spatial join operator**, a **GEOG type**, a **RASTER type**, **PostGIS-style topologies**, **pgRouting-style network routing**, and significant **performance optimizations** to the spatial join pipeline. The goal is PostGIS parity and SedonaDB-competitive performance within DuckDB's analytical engine.

**381 documented functions** (vs. 166 upstream) | **235 test files / 13267 assertions** | Synced with upstream v2.0-cyanoptera

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

`ST_ClusterDBSCAN` and `ST_ClusterKMeans` are upstream's window functions.

```sql
-- DBSCAN: find clusters with eps=100m, minpoints=5
SELECT *, ST_ClusterDBSCAN(geom, 100.0, 5) OVER (ORDER BY id) AS cluster_id
FROM retail_points;

-- K-means: partition into 10 clusters
SELECT *, ST_ClusterKMeans(geom, 10) OVER (ORDER BY id) AS cluster_id
FROM sensor_locations;
```

The fork adds the `ST_ClusterIntersecting` and `ST_ClusterWithin` aggregate functions.

## Performance

Measured against PostGIS 3.5 / pgRouting 4.0 on the same machine and the same data, this fork is faster on all 22 operations of the benchmark, on one core as well as on all cores, with identical results: see [docs/benchmarks.md](docs/benchmarks.md) for the queries, the numbers and the method.

- **Point-in-polygon joins**: `ST_Intersects`, `ST_Contains`, `ST_Within`, `ST_Covers` and `ST_CoveredBy` answer point-versus-polygon with an exact ray crossing count on the serialized polygon, without GEOS, and the spatial join keeps an index of the build-side polygons it tests (about 10x faster on one thread)
- **Parallel overlays**: `ST_Intersection`, `ST_Difference` and `ST_Union` compute the rows of a chunk as parallel tasks when the geometries are large, so a few thousand big polygons use all the cores instead of one
- **Spatial join pipeline**: envelope pre-check before R-tree descent, BFS-to-DFS traversal (better cache locality), Hilbert sort permutation for sequential row access (~1.7x measured), batch bbox extraction
- **R-tree STR bulk loading**: full Sort-Tile-Recursive packing for the persistent R-tree index, improving query-time fan-out
- **Hot-path cleanups**: `pow(x,2)` replaced with `x*x` across all distance kernels, `std::sort` replaces hand-rolled quicksort in FlatRTree
- **Robust predicates**: Shewchuk adaptive-precision `orient2d` for exact point-in-polygon and intersection tests near collinear edges

## Functions added to upstream

The fork only adds what upstream spatial and DuckDB do not provide. A function is kept when no upstream or DuckDB function computes the same thing, or when a measurement shows an upstream bug, a clear performance gain or results of unacceptable quality. The list below gives the reason for each one.

Functions of earlier versions of the fork that upstream or DuckDB provide since v2.0, with the call that replaces them. Their results were compared on 398 000 building footprints and points and on edge cases (empty, Z, M, collections): identical in every case.

| Removed | Use instead |
|---|---|
| `ST_AsEWKB(geom)` | `ST_AsWKB(geom)` |
| `ST_AsEWKT(geom)` | `ST_AsText(geom)` (it never wrote an `SRID=` prefix) |
| `ST_GeomFromEWKB(blob)` | `ST_GeomFromWKB(blob)`, which reads EWKB with an SRID |
| `ST_GeomFromEWKT(text)` | `ST_GeomFromText(text)`, which reads an `SRID=...;` prefix |
| `ST_Polygon(linestring)` | `ST_MakePolygon(linestring)` |
| `ST_NDims(geom)` | `ST_Dimension(geom)`, which it returned. The number of coordinates, as in PostGIS `ST_NDims`, is `ST_CoordDim(geom)` |
| `ST_SnapToGrid(geom, size)` | `ST_ReducePrecision(geom, size)` |
| `ST_SRID(geom)` | `ST_CRS(geom)`: it returned 0 for every geometry |
| `ST_SetSRID(geom, srid)` | `ST_SetCRS(geom, 'EPSG:' || srid)`: it returned the geometry unchanged |
| `ST_ClusterDBSCAN`, `ST_ClusterKMeans` | upstream's functions of the same name. DBSCAN gives the same clusters (border points within reach of two clusters may go to either), K-means starts from k-means++ and can converge to other clusters |

The `RASTER` overloads of `ST_Polygon`, `ST_SnapToGrid`, `ST_SRID` and `ST_SetSRID` are kept.

Kept functions and overloads:

- `CreateTopology`: PostGIS topology function
- `DropTopology`: PostGIS topology function
- `GetEdgeByPoint`: PostGIS topology function
- `GetFaceByPoint`: PostGIS topology function
- `GetNodeByPoint`: PostGIS topology function
- `GetNodeEdges`: PostGIS topology function
- `GetRingEdges`: PostGIS topology function
- `GetTopologyID`: PostGIS topology function
- `GetTopologyName`: PostGIS topology function
- `GetTopologySRID`: PostGIS topology function
- `pgr_analyzeGraph`: pgRouting function
- `pgr_aStar`: pgRouting function
- `pgr_bdAstar`: pgRouting function
- `pgr_bdDijkstra`: pgRouting function
- `pgr_boykovKolmogorov`: pgRouting function
- `pgr_connectedComponents`: pgRouting function
- `pgr_createTopology`: pgRouting function
- `pgr_dijkstra`: pgRouting function
- `pgr_dijkstraCost`: pgRouting function
- `pgr_dijkstraCostMatrix`: pgRouting function
- `pgr_drivingDistance`: pgRouting function
- `pgr_edmondsKarp`: pgRouting function
- `pgr_extractVertices`: pgRouting function
- `pgr_KSP`: pgRouting function
- `pgr_maxFlow`: pgRouting function
- `pgr_maxFlowMinCost`: pgRouting function
- `pgr_minCostMaxFlow`: pgRouting function
- `pgr_nodeNetwork`: pgRouting function
- `pgr_pushRelabel`: pgRouting function
- `pgr_strongComponents`: pgRouting function
- `pgr_trsp`: pgRouting function
- `pgr_TSP`: pgRouting function
- `pgr_withPoints`: pgRouting function
- `ST_3DDistance`: no upstream or DuckDB function computes it
- `ST_3DLength`: no upstream or DuckDB function computes it
- `ST_3DLineInterpolatePoint`: no upstream or DuckDB function computes it
- `ST_3DPerimeter`: no upstream or DuckDB function computes it
- `ST_AddBand`: RASTER type
- `ST_AddEdgeModFace`: PostGIS topology function
- `ST_AddEdgeNewFaces`: PostGIS topology function
- `ST_AddIsoEdge`: PostGIS topology function
- `ST_AddIsoNode`: PostGIS topology function
- `ST_AddMeasure`: no upstream or DuckDB function computes it
- `ST_AddPoint`: no upstream or DuckDB function computes it
- `ST_Angle`: no upstream or DuckDB function computes it
- `ST_AsEncodedPolyline`: no upstream or DuckDB function computes it
- `ST_AsGDALRaster`: RASTER type
- `ST_AsGML`: no upstream or DuckDB function computes it
- `ST_AsKML`: no upstream or DuckDB function computes it
- `ST_AsLatLonText`: no upstream or DuckDB function computes it
- `ST_Aspect`: RASTER type
- `ST_AsRaster`: RASTER type
- `ST_AsTIFF`: RASTER type
- `ST_AsTWKB`: no upstream or DuckDB function computes it
- `ST_Band`: RASTER type
- `ST_BandIsNoData`: RASTER type
- `ST_BandMetaData`: RASTER type
- `ST_BandNoDataValue`: RASTER type
- `ST_BandPixelType`: RASTER type
- `ST_BoundingDiagonal`: no upstream or DuckDB function computes it
- `ST_Box2dFromGeoHash`: no upstream or DuckDB function computes it
- `ST_ChaikinSmoothing`: no upstream or DuckDB function computes it
- `ST_ChangeEdgeGeom`: PostGIS topology function
- `ST_Clip`: RASTER type
- `ST_ClipByBox2D`: same result as `ST_Intersection` with the box, 1.9x faster (one thread, 398k buildings)
- `ST_ClusterIntersecting`: aggregate returning the clusters as collections; upstream only has window functions
- `ST_ClusterWithin`: aggregate returning the clusters as collections; upstream only has window functions
- `ST_ColorMap`: RASTER type
- `ST_Contour`: RASTER type
- `ST_CoordDim`: upstream only has `ST_ZMFlag`, a flag rather than the number of coordinates
- `ST_Count`: RASTER type
- `ST_CreateTopoGeo`: PostGIS topology function
- `ST_DelaunayTriangles`: no upstream or DuckDB function computes it
- `ST_DFullyWithin`: no upstream or DuckDB function computes it
- `ST_DumpAsPolygons`: RASTER type
- `ST_DumpPoints`: upstream `ST_Points` returns one MULTIPOINT, not one row per vertex with its path
- `ST_DumpRings`: no upstream or DuckDB function computes it
- `ST_DumpSegments`: no upstream or DuckDB function computes it
- `ST_DumpValues`: RASTER type
- `ST_ForceCollection`: no upstream or DuckDB function computes it
- `ST_ForcePolygonCCW`: no upstream or DuckDB function computes it
- `ST_ForcePolygonCW`: no upstream or DuckDB function computes it
- `ST_FrechetDistance`: no upstream or DuckDB function computes it
- `ST_FromGDALRaster`: RASTER type
- `ST_GDALDrivers`: RASTER type
- `ST_GeogFromText`: GEOG type
- `ST_GeogFromWKB`: GEOG type
- `ST_GeogFromWKT`: GEOG type
- `ST_GeogPoint`: GEOG type
- `ST_GeographyFromText`: GEOG type
- `ST_GeoHash`: no upstream or DuckDB function computes it
- `ST_GeometricMedian`: no upstream or DuckDB function computes it
- `ST_GeometryN`: upstream has no indexed access to the parts of a collection (only `ST_Dump`)
- `ST_GeomFromGeoHash`: no upstream or DuckDB function computes it
- `ST_GeomFromGML`: no upstream or DuckDB function computes it
- `ST_GeomFromKML`: no upstream or DuckDB function computes it
- `ST_GeomFromTWKB`: no upstream or DuckDB function computes it
- `ST_GeoReference`: RASTER type
- `ST_GetFaceEdges`: PostGIS topology function
- `ST_GetFaceGeometry`: PostGIS topology function
- `ST_Grayscale`: RASTER type
- `ST_HasNoBand`: RASTER type
- `ST_HausdorffDistance`: no upstream or DuckDB function computes it
- `ST_Height`: RASTER type
- `ST_HexagonGrid`: upstream `ST_AsGrid` returns cell centres, not cell polygons
- `ST_Hillshade`: RASTER type
- `ST_Histogram`: RASTER type
- `ST_InterpolateRaster`: RASTER type
- `ST_IsCollection`: no upstream or DuckDB function computes it
- `ST_IsPolygonCCW`: no upstream or DuckDB function computes it
- `ST_IsPolygonCW`: no upstream or DuckDB function computes it
- `ST_IsValidDetail`: no upstream or DuckDB function computes it
- `ST_IsValidReason`: no upstream or DuckDB function computes it
- `ST_KNN`: k-nearest-neighbour join operator
- `ST_LargestEmptyCircle`: no upstream or DuckDB function computes it
- `ST_LineFromEncodedPolyline`: no upstream or DuckDB function computes it
- `ST_LineFromMultiPoint`: upstream `ST_MakeLine` takes a list of points, not a MULTIPOINT
- `ST_LongestLine`: no upstream or DuckDB function computes it
- `ST_MakeEmptyRaster`: RASTER type
- `ST_MapAlgebra`: RASTER type
- `ST_MaxDistance`: no upstream or DuckDB function computes it
- `ST_MemSize`: no upstream or DuckDB function computes it
- `ST_MetaData`: RASTER type
- `ST_MinConvexHull`: RASTER type
- `ST_MinimumBoundingCircle`: no upstream or DuckDB function computes it
- `ST_MinimumClearance`: no upstream or DuckDB function computes it
- `ST_MinimumClearanceLine`: no upstream or DuckDB function computes it
- `ST_ModEdgeHeal`: PostGIS topology function
- `ST_ModEdgeSplit`: PostGIS topology function
- `ST_MoveIsoNode`: PostGIS topology function
- `ST_NearestValue`: RASTER type
- `ST_NewEdgeHeal`: PostGIS topology function
- `ST_NewEdgesSplit`: PostGIS topology function
- `ST_NotSameAlignmentReason`: RASTER type
- `ST_NRings`: upstream only counts interior rings (`ST_NumInteriorRings`)
- `ST_NumBands`: RASTER type
- `ST_OffsetCurve`: no upstream or DuckDB function computes it
- `ST_OrderingEquals`: no upstream or DuckDB function computes it
- `ST_PixelAsCentroid`: RASTER type
- `ST_PixelAsCentroids`: RASTER type
- `ST_PixelAsPoint`: RASTER type
- `ST_PixelAsPoints`: RASTER type
- `ST_PixelAsPolygon`: RASTER type
- `ST_PixelAsPolygons`: RASTER type
- `ST_PixelHeight`: RASTER type
- `ST_PixelWidth`: RASTER type
- `ST_Polygon`: RASTER type
- `ST_Project`: no upstream or DuckDB function computes it (GEOMETRY and GEOG overloads)
- `ST_Quantile`: RASTER type
- `ST_QuantizeCoordinates`: rounds to decimal places; differs from `ST_ReducePrecision` on every geometry tested
- `ST_RasterToWorldCoord`: RASTER type
- `ST_RasterToWorldCoordX`: RASTER type
- `ST_RasterToWorldCoordY`: RASTER type
- `ST_ReadRaster`: RASTER type
- `ST_Reclass`: RASTER type
- `ST_Relate`: no upstream or DuckDB function computes it
- `ST_RelateMatch`: no upstream or DuckDB function computes it
- `ST_RemEdgeModFace`: PostGIS topology function
- `ST_RemEdgeNewFace`: PostGIS topology function
- `ST_RemoveIsoEdge`: PostGIS topology function
- `ST_RemoveIsoNode`: PostGIS topology function
- `ST_RemovePoint`: no upstream or DuckDB function computes it
- `ST_Resample`: RASTER type
- `ST_Rescale`: RASTER type
- `ST_Resize`: RASTER type
- `ST_Reskew`: RASTER type
- `ST_Retile`: RASTER type
- `ST_Rotation`: RASTER type
- `ST_Roughness`: RASTER type
- `ST_SameAlignment`: RASTER type
- `ST_ScaleX`: RASTER type
- `ST_ScaleY`: RASTER type
- `ST_Scroll`: no upstream or DuckDB function computes it
- `ST_Segmentize`: no upstream or DuckDB function computes it (GEOMETRY and GEOG overloads)
- `ST_SetBandNoDataValue`: RASTER type
- `ST_SetGeoReference`: RASTER type
- `ST_SetPoint`: no upstream or DuckDB function computes it
- `ST_SetScale`: RASTER type
- `ST_SetSkew`: RASTER type
- `ST_SetSRID`: RASTER type
- `ST_SetUpperLeft`: RASTER type
- `ST_SetValue`: RASTER type
- `ST_SharedPaths`: no upstream or DuckDB function computes it
- `ST_ShiftLongitude`: no upstream or DuckDB function computes it
- `ST_SimplifyPolygonHull`: no upstream or DuckDB function computes it
- `ST_SimplifyVW`: no upstream or DuckDB function computes it
- `ST_SkewX`: RASTER type
- `ST_SkewY`: RASTER type
- `ST_Slope`: RASTER type
- `ST_SnapToGrid`: RASTER type
- `ST_Split`: no upstream or DuckDB function computes it
- `ST_SquareGrid`: upstream `ST_AsGrid` returns cell centres, not cell polygons
- `ST_SRID`: RASTER type
- `ST_Summary`: no upstream or DuckDB function computes it
- `ST_SummaryStats`: RASTER type
- `ST_SummaryStatsAgg`: RASTER type
- `ST_SwapOrdinates`: `ST_FlipCoordinates` only swaps x and y; this also swaps z and m
- `ST_Tile`: RASTER type
- `ST_TPI`: RASTER type
- `ST_TRI`: RASTER type
- `ST_TriangulatePolygon`: no upstream or DuckDB function computes it
- `ST_UnaryUnion`: upstream only has the binary `ST_Union` and the aggregates
- `ST_UpperLeftX`: RASTER type
- `ST_UpperLeftY`: RASTER type
- `ST_Value`: RASTER type
- `ST_ValueCount`: RASTER type
- `ST_Width`: RASTER type
- `ST_WorldToRasterCoord`: RASTER type
- `ST_WorldToRasterCoordX`: RASTER type
- `ST_WorldToRasterCoordY`: RASTER type
- `TopoElementArray_Agg`: PostGIS topology function
- `TopologySummary`: PostGIS topology function
- `ValidateTopology`: PostGIS topology function
- `ST_Area` (GEOG overloads): GEOG type
- `st_astext` (GEOG overloads): GEOG type
- `st_aswkb` (GEOG overloads): GEOG type
- `ST_Azimuth` (GEOG overloads): GEOG type
- `ST_Buffer` (GEOG overloads): GEOG type
- `ST_Contains` (RASTER overloads): RASTER type
- `ST_ConvexHull` (RASTER overloads): RASTER type
- `st_crs` (RASTER overloads): RASTER type
- `ST_Distance` (GEOG overloads): GEOG type
- `ST_DWithin` (GEOG overloads): GEOG type
- `ST_Envelope` (RASTER overloads): RASTER type
- `ST_Intersection` (RASTER overloads): RASTER type
- `ST_Intersects` (GEOG, RASTER overloads): GEOG type and RASTER type
- `ST_Length` (GEOG overloads): GEOG type
- `ST_Overlaps` (RASTER overloads): RASTER type
- `ST_Perimeter` (GEOG overloads): GEOG type
- `st_setcrs` (RASTER overloads): RASTER type
- `ST_Touches` (RASTER overloads): RASTER type
- `ST_Transform` (RASTER overloads): RASTER type
- `ST_Union_Agg` (RASTER overloads): RASTER type
- `ST_Within` (RASTER overloads): RASTER type

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

## DuckDB version and releases

The extension is built against the DuckDB in the `duckdb` submodule, currently **v2.0.0**, and only works in that version of DuckDB: it uses DuckDB's internal C++ API, so a binary built from a newer DuckDB can be labelled for an older one (with `OVERRIDE_GIT_DESCRIBE`) and still crash when it is loaded there. The build refuses such a label unless `-DSPATIAL_ALLOW_OLDER_DUCKDB_LABEL=ON` is passed. Every change of the submodule is noted here.

| Fork commits | DuckDB |
|---|---|
| from `75a6f54e` (2026-10-28) | v2.0.0 |
| from `d16a73c2` (2026-09-30) | v1.5.6 |
| up to `b16c974d` | v1.5.4 |

Validated commits are tagged `v<duckdb version>-fork.<n>` (for example `v1.5.6-fork.1`): pin one of those rather than a branch head.

Each DuckDB release, patch releases included, needs a rebuild. `scripts/rebuild.sh v2.0.1` moves the submodule to the tag, builds DuckDB, the extension and its vcpkg dependencies for x86-64-v3 (with the vcpkg commit of the CI), checks the version the build reports, and runs the tests. `MERGE_UPSTREAM=upstream/v2.0-cyanoptera` merges the upstream branch first, `MARCH=x86-64` builds for the generic baseline and `NO_TESTS=1` skips the tests.

Since DuckDB v2.0, the functions that depend on the axis order of their input (`ST_Transform` with CRS strings, the `_Spheroid` functions, `ST_Distance_Sphere`, `COPY ... (FORMAT GDAL)` with an SRS) fail unless `geometry_always_xy` is set explicitly, to `true` for longitude/latitude input or to `false` for latitude/longitude.

## Using the build from another DuckDB client

The build also produces a loadable extension, laid out as a local extension repository in `build/release/repository`. It can only be loaded by a DuckDB of the exact version it was built against (currently v2.0.0), on the same platform.

The binary is not signed, and it cannot be: DuckDB only trusts the keys compiled into it. A connection loads it only if it is opened with unsigned extensions allowed, which cannot be changed once the database is open:

| Client | Setting |
|---|---|
| CLI | `duckdb -unsigned` |
| Python | `duckdb.connect(config={'allow_unsigned_extensions': 'true'})` |
| C API | `duckdb_set_config(config, "allow_unsigned_extensions", "true")` before `duckdb_open_ext` |
| Rust | `Connection::open_with_flags(path, Config::default().allow_unsigned_extensions()?)` |

**Keep it out of the shared extension directory.** `~/.duckdb/extensions` is shared by every DuckDB client of the user. A client that checks signatures (the default) cannot load the build from there, and any tool that answers a failed `LOAD` with `FORCE INSTALL spatial` replaces it with the official extension, after which the fork's code is silently gone. Give the workflows that want the fork an extension directory of their own instead:

```bash
make install-local    # installs into ~/.duckdb/extensions-spatial-fork (LOCAL_EXTENSION_DIRECTORY=... to change it)
```

and open their connections with that directory and unsigned extensions allowed:

```sql
-- CLI: duckdb -unsigned
SET extension_directory = '/home/<user>/.duckdb/extensions-spatial-fork';
LOAD spatial;
SELECT extension_version, install_mode FROM duckdb_extensions() WHERE extension_name = 'spatial';   -- the commit of this repository, REPOSITORY
```

```python
duckdb.connect(config={'allow_unsigned_extensions': 'true',
                       'extension_directory': os.path.expanduser('~/.duckdb/extensions-spatial-fork')})
```

Other extensions (`parquet`, `httpfs`, ...) are installed into that directory as needed, like in the default one. Loading the file by its full path (`LOAD '/path/to/spatial.duckdb_extension'`, still with unsigned extensions allowed) works too and involves no extension directory at all.

Do not copy `spatial.duckdb_extension` over an installed one by hand: the `.info` file next to it still describes the previous binary, and loading by name then fails with `Metadata mismatch detected when loading extension`.

# Example Usage

See the [example](docs/example.md) for basic usage.

# Supported Functions and Documentation

The full list of functions and their documentation is available in the [function reference](docs/functions.md).

# Internals and Technical Details

See the [internals documentation](docs/internals.md) for details on the internal workings of the extension.
