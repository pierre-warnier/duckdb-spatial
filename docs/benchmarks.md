# Benchmarks against PostGIS

This page compares the extension with PostGIS on the operations where both do the same thing: vector predicates and joins, geodesic distance, clustering, overlays, routing, raster and topology. Every query returns the same result on both sides, and the comparison checks it.

## Setup

- **Machine**: one workstation, Intel i9-14900F (32 threads), 188 GB of RAM. Both engines run on it and read the same data. It is a shared machine and was busy during the run: each figure is the best of three passes within a single run of the benchmark, and the ratios are more stable than the absolute times.
- **DuckDB** v1.5.6 with this extension, release build. Tables are stored in a DuckDB file.
- **PostgreSQL 17, PostGIS 3.5.2, pgRouting 4.0.1** (`pgrouting/pgrouting:17-3.5-4.0.1` image), with 8 GB of shared buffers, 1 GB of work memory, GiST indexes on every geometry column, `ANALYZE` run, and up to 8 parallel workers per query.
- **Data**: Belgian addresses (6.9 million points), street sections (286 000 points), statistical sectors (2614 polygons, 1.9 million vertices) and municipalities (565 polygons, 1.4 million vertices), in longitude/latitude. The routing graph is a 700 x 700 grid (980 000 edges) with random costs; the raster is a 4096 x 4096 analytic surface cut into 256 tiles of 256 x 256.
- "1 core" is `SET threads = 1` in DuckDB and `max_parallel_workers_per_gather = 0` in PostgreSQL. "All cores" is the best time over 1, 8 and 32 threads for DuckDB, and over 0 and 8 parallel workers for PostgreSQL. pgRouting, raster aggregates and topology functions do not run in parallel in PostgreSQL.

## Results

Times in seconds. The last two columns are how many times faster the extension is.

| Operation | DuckDB 1 core | PostGIS 1 core | DuckDB all cores | PostGIS all cores | 1 core | All cores |
|---|---|---|---|---|---|---|
| Reproject 6.9M points | 2.04 | 3.62 | 0.24 | 1.43 | 1.8x | 5.9x |
| Point-in-polygon join, 6.9M x 2614 | 15.19 | 20.85 | 0.99 | 5.90 | 1.4x | 5.9x |
| KNN join, k = 1, 680k x 286k | 5.70 | 47.99 | 0.48 | 18.85 | 8.4x | 39x |
| KNN join, k = 5 | 4.82 | 21.20 | 0.33 | 20.43 | 4.4x | 63x |
| Geodesic distance, 6.9M point pairs | 3.66 | 7.15 | 0.39 | 1.11 | 2.0x | 2.9x |
| Geodesic distance, 67k points to a 9680-vertex line | 6.25 | 63.04 | 0.94 | 62.71 | 10x | 66x |
| DBSCAN, 680k points | 0.93 | 2.66 | 0.88 | 2.40 | 2.8x | 2.7x |
| Buffer and union, 28.6k points | 2.08 | 3.96 | 2.06 | 3.87 | 1.9x | 1.9x |
| Polygon overlay, 2614 x 565 (8355 intersections) | 5.65 | 10.56 | 0.89 | 4.40 | 1.9x | 4.9x |
| Dijkstra, one to one | 0.14 | 0.76 | 0.12 | 0.76 | 5.3x | 6.2x |
| Dijkstra costs, 20 x 20 | 0.95 | 2.15 | 0.91 | 2.15 | 2.3x | 2.4x |
| Driving distance | 0.13 | 24.85 | 0.10 | 24.85 | 196x | 248x |
| Connected components | 0.13 | 0.78 | 0.10 | 0.78 | 6.2x | 7.7x |
| 5 shortest paths | 3.97 | 9.21 | 3.97 | 9.21 | 2.3x | 2.3x |
| Raster statistics, 256 tiles | 0.09 | 0.93 | 0.09 | 0.92 | 11x | 11x |
| Raster map algebra, 256 tiles | 0.27 | 13.31 | 0.27 | 11.81 | 50x | 44x |
| Raster slope, 256 tiles | 0.40 | 47.09 | 0.40 | 47.09 | 118x | 118x |
| Raster value at 200k points | 3.73 | 5.93 | 2.45 | 5.93 | 1.6x | 2.4x |
| Raster union of 256 tiles | 0.31 | 2.69 | 0.31 | 2.69 | 8.8x | 8.8x |
| Raster polygonize after reclass | 0.04 | 0.13 | 0.04 | 0.12 | 3.4x | 3.1x |
| Topology from 132 polygons (100k vertices) | 0.16 | 17.28 | 0.16 | 17.28 | 105x | 105x |
| Topology validation | 0.12 | 0.23 | 0.12 | 0.23 | 1.8x | 1.8x |

The smallest margins are the point-in-polygon join on one core (1.4x) and the raster lookup at points on one core (1.6x).

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
