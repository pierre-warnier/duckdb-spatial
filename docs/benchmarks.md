# Benchmarks against PostGIS

This page compares the extension with PostGIS on the operations where both do the same thing: vector predicates and joins, geodesic distance, clustering, overlays, routing, raster and topology. Every query returns the same result on both sides, and the comparison checks it.

## Setup

- **Machine**: one workstation, Intel i9-14900F (32 threads), 188 GB of RAM. Both engines run on it and read the same data. It is a shared machine and was busy during the runs: each figure is the best of several passes, and the ratios are more stable than the absolute times.
- **DuckDB** v1.5.6 with this extension, release build. Tables are stored in a DuckDB file.
- **PostgreSQL 17, PostGIS 3.5.2, pgRouting 4.0.1** (`pgrouting/pgrouting:17-3.5-4.0.1` image), with 8 GB of shared buffers, 1 GB of work memory, GiST indexes on every geometry column, `ANALYZE` run, and up to 8 parallel workers per query.
- **Data**: Belgian addresses (6.9 million points), street sections (286 000 points), statistical sectors (2614 polygons, 1.9 million vertices) and municipalities (565 polygons, 1.4 million vertices), in longitude/latitude. The routing graph is a 700 x 700 grid (980 000 edges) with random costs; the raster is a 4096 x 4096 analytic surface cut into 256 tiles of 256 x 256.
- "1 core" is `SET threads = 1` in DuckDB and `max_parallel_workers_per_gather = 0` in PostgreSQL. "All cores" is the best of 8 and 32 threads for DuckDB and 8 parallel workers for PostgreSQL. pgRouting, raster aggregates and topology functions do not run in parallel in PostgreSQL.

## Results

Times in seconds. The last two columns are how many times faster the extension is.

| Operation | DuckDB 1 core | PostGIS 1 core | DuckDB all cores | PostGIS all cores | 1 core | All cores |
|---|---|---|---|---|---|---|
| Reproject 6.9M points | 1.59 | 3.81 | 0.25 | 0.68 | 2.4x | 2.7x |
| Point-in-polygon join, 6.9M x 2614 | 6.33 | 9.80 | 0.56 | 2.40 | 1.5x | 4.3x |
| KNN join, k = 1, 680k x 286k | 2.93 | 18.60 | 0.27 | 18.45 | 6.3x | 68x |
| KNN join, k = 5 | 3.89 | 22.52 | 0.33 | 22.52 | 5.8x | 68x |
| Geodesic distance, 6.9M point pairs | 4.81 | 7.24 | 0.52 | 1.14 | 1.5x | 2.2x |
| Geodesic distance, 67k points to a 9680-vertex line | 6.59 | 69.47 | 1.38 | 69.47 | 10.5x | 50x |
| DBSCAN, 680k points | 0.94 | 3.06 | 0.91 | 2.55 | 3.3x | 2.8x |
| Buffer and union, 28.6k points | 2.17 | 4.14 | 2.17 | 3.94 | 1.9x | 1.8x |
| Polygon overlay, 2614 x 565 (8355 intersections) | 5.82 | 11.05 | 0.97 | 3.63 | 1.9x | 3.7x |
| Dijkstra, one to one | 0.15 | 0.77 | 0.12 | 0.77 | 5.1x | 6.4x |
| Dijkstra costs, 20 x 20 | 0.99 | 4.00 | 0.92 | 4.00 | 4.0x | 4.3x |
| Driving distance | 0.16 | 26.85 | 0.14 | 26.85 | 168x | 192x |
| Connected components | 0.14 | 0.81 | 0.11 | 0.81 | 5.8x | 7.4x |
| 5 shortest paths | 4.23 | 9.25 | 4.03 | 9.25 | 2.2x | 2.3x |
| Raster statistics, 256 tiles | 0.09 | 0.97 | 0.09 | 0.97 | 11x | 11x |
| Raster map algebra, 256 tiles | 0.29 | 9.82 | 0.27 | 9.82 | 34x | 36x |
| Raster slope, 256 tiles | 0.37 | 49.69 | 0.37 | 48.18 | 134x | 130x |
| Raster value at 200k points | 3.67 | 5.86 | 2.45 | 5.77 | 1.6x | 2.4x |
| Raster union of 256 tiles | 0.33 | 2.64 | 0.33 | 2.64 | 8.0x | 8.0x |
| Raster polygonize after reclass | 0.04 | 0.12 | 0.04 | 0.12 | 3.0x | 3.0x |
| Topology from 132 polygons (100k vertices) | 0.29 | 36.81 | 0.29 | 36.81 | 125x | 125x |
| Topology validation | 0.12 | 0.29 | 0.12 | 0.29 | 2.4x | 2.4x |

Where PostgreSQL gains nothing from parallel workers, its single-core time is repeated in the "all cores" column.

## Reading the numbers

- **Small margins on one core** (reprojection, point pairs, buffers, overlays) are operations where both engines call the same library (PROJ, GeographicLib, GEOS): the difference is the cost around the call.
- **KNN**: PostGIS runs one index-ordered scan per probe row (`ORDER BY geom <-> point LIMIT k` in a lateral join), which PostgreSQL does not parallelize. The extension builds one R-tree and probes it from every thread.
- **Point-in-polygon** is answered without GEOS, and the join keeps an index of the polygons it has tested. See [internals](internals.md#point-in-polygon).
- **Overlay**: DuckDB only parallelizes over the rows of the scanned tables, so the overlay functions spread the rows of a chunk over the threads themselves when the geometries are large.
- **Driving distance, raster slope and map algebra, topology construction** are implemented in PL/pgSQL or row by row in PostGIS and pgRouting; the large factors come from that, not from hardware.

## Differences in results

- **Geodesic distance to a line**: both sides are compared with GeographicLib, minimizing the distance to each edge. The extension agrees to a few nanometers. PostGIS is off by more than 1 cm for 39 of 6702 points, and by up to 29.7 m, because it locates the nearest point on a sphere before measuring on the ellipsoid.
- **DBSCAN**: noise points and core points are identical. Border points that are within reach of two clusters may be given to either, as the algorithm allows (54 points out of 66 946).
- **Raster slope** differs on the outermost row and column of each tile, where the two engines extrapolate differently (about 0.5% on the mean slope of a tile).
- **Topology validation**: PostGIS reports "face has wrong mbr" for every face of the topology it has just built from longitude/latitude polygons. The extension validates its own topology without errors. Node, edge and face counts are identical.

## Reproducing

`benchmark/postgis/bench_postgis.py` runs every query on both sides, compares the results and prints one JSON line per operation. It expects a DuckDB file (`BENCH_DB`) with the tables `padr`, `ss`, `nis6` and `muni`, a PostgreSQL server (`BENCH_PGHOST`, `BENCH_PGPORT`, `BENCH_PGPASSWORD`) holding the same tables in the schema `geoadmin` with the extensions `postgis`, `postgis_raster`, `postgis_topology` and `pgrouting`, and the path of the built extension (`SPATIAL_EXTENSION`). `ONLY=<substring>` restricts the run to matching operations and `RUNS` sets the number of passes. The address and boundary data are not public; any point and polygon tables of similar size will do.
