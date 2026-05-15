# DuckDB Spatial Extension (Enhanced Fork)

This fork of [duckdb/duckdb-spatial](https://github.com/duckdb/duckdb-spatial) extends the DuckDB spatial extension with **87 additional functions**, a **native KNN spatial join operator**, **DBSCAN/K-means clustering**, and significant **performance optimizations** to the spatial join pipeline. The goal is PostGIS parity and SedonaDB-competitive performance within DuckDB's analytical engine.

**241 spatial functions** (vs. 158 upstream) | **156 tests / 2133 assertions** | Synced with upstream v1.5-variegata

**Table of contents**
- [What's new in this fork](#whats-new-in-this-fork)
- [What is this?](#what-is-this)
- [How do I get it?](#how-do-i-get-it)
- [Example Usage](#example-usage)
- [Supported Functions](#supported-functions-and-documentation)
- [Internals and Technical Details](#internals-and-technical-details)

# What's new in this fork

## KNN Spatial Join

Native k-nearest-neighbor spatial join via `ST_KNN`, using Hjaltason-Samet priority-queue traversal over a FlatRTree. Includes over-fetch with exact distance refinement, haversine spheroidal distance, LEFT JOIN support, and spill-to-disk for larger-than-memory build sides.

```sql
-- Find 5 nearest hydrants for each building
SELECT b.id, h.id, ST_Distance(b.geom, h.geom) AS dist
FROM buildings b
JOIN hydrants h ON ST_KNN(b.geom, h.geom, 5);
```

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

- **Spatial join pipeline**: envelope pre-check before R-tree descent, BFS-to-DFS traversal (better cache locality), Hilbert sort permutation for sequential row access (~1.7x measured), batch bbox extraction
- **R-tree STR bulk loading**: full Sort-Tile-Recursive packing for the persistent R-tree index, improving query-time fan-out
- **Hot-path cleanups**: `pow(x,2)` replaced with `x*x` across all distance kernels, `std::sort` replaces hand-rolled quicksort in FlatRTree
- **Robust predicates**: Shewchuk adaptive-precision `orient2d` replaces the fast-but-wrong `orient2d_fast`, eliminating false positives in point-in-polygon and intersection tests near collinear edges
- **Native ST_Intersects**: GEOMETRY-to-GEOMETRY intersection without GEOS fallback for the common bbox-miss and point-in-polygon cases

## 87 New Functions (PostGIS parity)

| Category | Functions |
|---|---|
| **Serialization** (12) | `ST_AsEWKB`, `ST_AsEWKT`, `ST_AsTWKB`, `ST_GeomFromEWKB`, `ST_GeomFromEWKT`, `ST_GeomFromTWKB`, `ST_AsEncodedPolyline`, `ST_LineFromEncodedPolyline`, `ST_GeoHash`, `ST_GeomFromGeoHash`, `ST_Box2dFromGeoHash`, `ST_AsLatLonText` |
| **GEOS Construction** (16) | `ST_ClipByBox2D`, `ST_DelaunayTriangles`, `ST_GeometricMedian`, `ST_LargestEmptyCircle`, `ST_MinimumBoundingCircle`, `ST_MinimumClearance`, `ST_MinimumClearanceLine`, `ST_OffsetCurve`, `ST_SharedPaths`, `ST_SimplifyPolygonHull`, `ST_Snap`, `ST_Split`, `ST_Subdivide`, `ST_TriangulatePolygon`, `ST_UnaryUnion`, `ST_CoverageClean` |
| **Geometry Editing** (15) | `ST_AddPoint`, `ST_SetPoint`, `ST_RemovePoint`, `ST_ChaikinSmoothing`, `ST_ForceCollection`, `ST_QuantizeCoordinates`, `ST_Scroll`, `ST_Segmentize`, `ST_SetSRID`, `ST_ShiftLongitude`, `ST_SimplifyVW`, `ST_SwapOrdinates`, `ST_ForcePolygonCCW`, `ST_ForcePolygonCW`, `ST_SnapToGrid` |
| **Accessors** (12) | `ST_BoundingDiagonal`, `ST_GeometryN`, `ST_InteriorRingN`, `ST_IsCollection`, `ST_IsPolygonCCW`, `ST_IsPolygonCW`, `ST_IsValidDetail`, `ST_IsValidReason`, `ST_MemSize`, `ST_NRings`, `ST_SRID`, `ST_Summary` |
| **3D / Measure** (8) | `ST_3DDistance`, `ST_3DLength`, `ST_3DLineInterpolatePoint`, `ST_3DPerimeter`, `ST_AddMeasure`, `ST_CoordDim`, `ST_NDims`, `ST_SwapOrdinates` |
| **Distance / Proximity** (6) | `ST_Angle`, `ST_ClosestPoint`, `ST_FrechetDistance`, `ST_HausdorffDistance`, `ST_LongestLine`, `ST_MaxDistance` |
| **Clustering** (4) | `ST_ClusterDBSCAN`, `ST_ClusterIntersecting`, `ST_ClusterKMeans`, `ST_ClusterWithin` |
| **Predicates** (4) | `ST_DFullyWithin`, `ST_OrderingEquals`, `ST_Relate`, `ST_RelateMatch` |
| **Decomposition** (3) | `ST_DumpPoints`, `ST_DumpRings`, `ST_DumpSegments` |
| **Constructors** (2) | `ST_LineFromMultiPoint`, `ST_Polygon` |
| **Grids** (2) | `ST_HexagonGrid`, `ST_SquareGrid` |
| **Geodesic** (2) | `ST_Project`, `ST_Expand` |
| **Join** (1) | `ST_KNN` |

## Backward Compatibility

Databases written by duckdb-spatial before DuckDB v1.5 (when GEOMETRY was a BLOB alias) are automatically readable. The extension registers an implicit cast that walks the legacy binary format and reserializes to the native GEOMETRY layout.

## Correctness Fixes

- GEOS deserialization alignment fix (double-aligned buffers for `GEOSCoordSeq_copyFromBuffer_r`)
- `ST_ForceCollection` deep copy for nested multi-part geometries
- `ST_3DDistance` / `ST_DFullyWithin` restricted to POINT inputs (vertex-only computation is incorrect for lines/polygons)
- `ST_Intersects` fallback uses exact distance instead of bbox-only heuristic
- `robust::init()` thread safety via `std::call_once`
- Spatial join dirty validity mask fix (cherry-picked from upstream #812)

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

# Example Usage

See the [example](docs/example.md) for basic usage.

# Supported Functions and Documentation

The full list of functions and their documentation is available in the [function reference](docs/functions.md).

# Internals and Technical Details

See the [internals documentation](docs/internals.md) for details on the internal workings of the extension.
