# Spatial Internals

> The first sections below come from the upstream extension and predate DuckDB v1.5. Since v1.5 `GEOMETRY` is a type of DuckDB itself (stored as WKB, optionally carrying a coordinate reference system) rather than a `BLOB` alias defined by this extension; the columnar `POINT_2D`, `LINESTRING_2D`, `POLYGON_2D` and `BOX_2D` types are unchanged. The [last section](#additions-of-this-fork) describes what this fork adds.

## Multi-tiered Geometry Type System
This extension implements 5 different geometry types. Like almost all geospatial databases we include a `GEOMETRY` type that (at least strives) to follow the Simple Features geometry model. This includes support for the standard subtypes, such as `POINT`, `LINESTRING`, `POLYGON`, `MULTIPOINT`, `MULTILINESTRING`, `MULTIPOLYGON`, `GEOMETRYCOLLECTION` that we all know and love, internally represented in a row-wise fashion on top of DuckDB `BLOB`s. The internal binary format is very similar to the one used by PostGIS - basically `double` aligned WKB, and we may eventually look into enforcing the format to be properly compatible with PostGIS (which may be useful for the PostGIS scanner extension). Most functions that are implemented for this type uses the [GEOS library](https://github.com/libgeos/geos), which is a battle-tested C++ port of the famous `JTS` library, to perform the actual operations on the geometries.

While having a flexible and dynamic `GEOMETRY` type is great to have, it is comparatively rare to work with columns containing mixed-geometries after the initial import and cleanup step. In fact, in most OLAP use cases you will probably only have a single geometry type in a table, and in those cases you're paying the performance cost to de/serialize and branch on the internal geometry format unneccessarily, i.e. you're paying for flexibility you're not using. For those cases we implement a set of non-standard DuckDB "native" geometry types, `POINT_2D`, `LINESTRING_2D`, `POLYGON_2D`, and `BOX_2D`. These types are built on DuckDBs `STRUCT` and `LIST` types, and are stored in a columnar fashion with the coordinate dimensions stored in separate "vectors". This makes it possible to leverage DuckDB's per-column statistics, compress much more efficiently and perform spatial operations on these geometries without having to de/serialize them first. Storing the coordinate dimensions into separate vectors also allows casting and converting between geometries with multiple different dimensions basically for free. And if you truly need to mix a couple of different geometry types, you can always use a DuckDB [UNION type](https://duckdb.org/docs/sql/data_types/union).

For now only a small amount of spatial functions are overloaded for these native types, but since they can be implicitly cast to `GEOMETRY` you can always use any of the functions that are implemented for `GEOMETRY` on them as well in the meantime while we work on adding more (although with a de/serialization penalty).

This extension also includes a `WKB_BLOB` type as an alias for `BLOB` that is used to indicate that the blob contains valid WKB encoded geometry.

## Per-thread Arena Allocation for Geometry Objects
When materializing the `GEOMETRY` type objects from the internal binary format we use per-thread arena allocation backed by DuckDB's buffer manager to amortize the contention and performance cost of performing lots of small heap allocations and frees, which allows us to utilizes DuckDB's multi-threaded vectorized out-of-core execution fully. While most spatial functions are implemented by wrapping `GEOS`, which requires an extra copy/allocation step anyway, the plan is to incrementally implementat our own versions of the simpler functions that can operate directly on our own `GEOMETRY` representation in order to greatly accelerate geospatial processing.

## Embedded PROJ Database
[PROJ](https://proj.org/#) is a generic coordinate transformation library that transforms geospatial coordinates from one projected coordinate reference system (CRS) to another. This extension experiments with including an embedded version of the PROJ database inside the extension binary itself so that you don't have to worry about installing the PROJ library separately. This also opens up the possibility to use this functionality in WASM.

## Embedded GDAL based Input/Output Functions
[GDAL](https://github.com/OSGeo/gdal) is a translator library for raster and vector geospatial data formats. This extension includes and exposes a subset of the GDAL vector drivers through the `ST_Read` and `COPY ... TO ... WITH (FORMAT GDAL)` table and copy functions respectively to read and write geometry data from and to a variety of file formats as if they were DuckDB tables. We currently support the over 50 GDAL formats - check for yourself by running
<details>
<summary>
    SELECT * FROM st_drivers();
</summary>

|   short_name   |                      long_name                       | can_create | can_copy | can_open |                      help_url                      |
|----------------|------------------------------------------------------|------------|----------|----------|----------------------------------------------------|
| ESRI Shapefile | ESRI Shapefile                                       | true       | false    | true     | https://gdal.org/drivers/vector/shapefile.html     |
| MapInfo File   | MapInfo File                                         | true       | false    | true     | https://gdal.org/drivers/vector/mitab.html         |
| UK .NTF        | UK .NTF                                              | false      | false    | true     | https://gdal.org/drivers/vector/ntf.html           |
| LVBAG          | Kadaster LV BAG Extract 2.0                          | false      | false    | true     | https://gdal.org/drivers/vector/lvbag.html         |
| S57            | IHO S-57 (ENC)                                       | true       | false    | true     | https://gdal.org/drivers/vector/s57.html           |
| DGN            | Microstation DGN                                     | true       | false    | true     | https://gdal.org/drivers/vector/dgn.html           |
| OGR_VRT        | VRT - Virtual Datasource                             | false      | false    | true     | https://gdal.org/drivers/vector/vrt.html           |
| Memory         | Memory                                               | true       | false    | true     |                                                    |
| CSV            | Comma Separated Value (.csv)                         | true       | false    | true     | https://gdal.org/drivers/vector/csv.html           |
| GML            | Geography Markup Language (GML)                      | true       | false    | true     | https://gdal.org/drivers/vector/gml.html           |
| GPX            | GPX                                                  | true       | false    | true     | https://gdal.org/drivers/vector/gpx.html           |
| KML            | Keyhole Markup Language (KML)                        | true       | false    | true     | https://gdal.org/drivers/vector/kml.html           |
| GeoJSON        | GeoJSON                                              | true       | false    | true     | https://gdal.org/drivers/vector/geojson.html       |
| GeoJSONSeq     | GeoJSON Sequence                                     | true       | false    | true     | https://gdal.org/drivers/vector/geojsonseq.html    |
| ESRIJSON       | ESRIJSON                                             | false      | false    | true     | https://gdal.org/drivers/vector/esrijson.html      |
| TopoJSON       | TopoJSON                                             | false      | false    | true     | https://gdal.org/drivers/vector/topojson.html      |
| OGR_GMT        | GMT ASCII Vectors (.gmt)                             | true       | false    | true     | https://gdal.org/drivers/vector/gmt.html           |
| GPKG           | GeoPackage                                           | true       | true     | true     | https://gdal.org/drivers/vector/gpkg.html          |
| SQLite         | SQLite / Spatialite                                  | true       | false    | true     | https://gdal.org/drivers/vector/sqlite.html        |
| WAsP           | WAsP .map format                                     | true       | false    | true     | https://gdal.org/drivers/vector/wasp.html          |
| OpenFileGDB    | ESRI FileGDB                                         | true       | false    | true     | https://gdal.org/drivers/vector/openfilegdb.html   |
| DXF            | AutoCAD DXF                                          | true       | false    | true     | https://gdal.org/drivers/vector/dxf.html           |
| CAD            | AutoCAD Driver                                       | false      | false    | true     | https://gdal.org/drivers/vector/cad.html           |
| FlatGeobuf     | FlatGeobuf                                           | true       | false    | true     | https://gdal.org/drivers/vector/flatgeobuf.html    |
| Geoconcept     | Geoconcept                                           | true       | false    | true     |                                                    |
| GeoRSS         | GeoRSS                                               | true       | false    | true     | https://gdal.org/drivers/vector/georss.html        |
| VFK            | Czech Cadastral Exchange Data Format                 | false      | false    | true     | https://gdal.org/drivers/vector/vfk.html           |
| PGDUMP         | PostgreSQL SQL dump                                  | true       | false    | false    | https://gdal.org/drivers/vector/pgdump.html        |
| OSM            | OpenStreetMap XML and PBF                            | false      | false    | true     | https://gdal.org/drivers/vector/osm.html           |
| GPSBabel       | GPSBabel                                             | true       | false    | true     | https://gdal.org/drivers/vector/gpsbabel.html      |
| WFS            | OGC WFS (Web Feature Service)                        | false      | false    | true     | https://gdal.org/drivers/vector/wfs.html           |
| OAPIF          | OGC API - Features                                   | false      | false    | true     | https://gdal.org/drivers/vector/oapif.html         |
| EDIGEO         | French EDIGEO exchange format                        | false      | false    | true     | https://gdal.org/drivers/vector/edigeo.html        |
| SVG            | Scalable Vector Graphics                             | false      | false    | true     | https://gdal.org/drivers/vector/svg.html           |
| ODS            | Open Document/ LibreOffice / OpenOffice Spreadsheet  | true       | false    | true     | https://gdal.org/drivers/vector/ods.html           |
| XLSX           | MS Office Open XML spreadsheet                       | true       | false    | true     | https://gdal.org/drivers/vector/xlsx.html          |
| Elasticsearch  | Elastic Search                                       | true       | false    | true     | https://gdal.org/drivers/vector/elasticsearch.html |
| Carto          | Carto                                                | true       | false    | true     | https://gdal.org/drivers/vector/carto.html         |
| AmigoCloud     | AmigoCloud                                           | true       | false    | true     | https://gdal.org/drivers/vector/amigocloud.html    |
| SXF            | Storage and eXchange Format                          | false      | false    | true     | https://gdal.org/drivers/vector/sxf.html           |
| Selafin        | Selafin                                              | true       | false    | true     | https://gdal.org/drivers/vector/selafin.html       |
| JML            | OpenJUMP JML                                         | true       | false    | true     | https://gdal.org/drivers/vector/jml.html           |
| PLSCENES       | Planet Labs Scenes API                               | false      | false    | true     | https://gdal.org/drivers/vector/plscenes.html      |
| CSW            | OGC CSW (Catalog  Service for the Web)               | false      | false    | true     | https://gdal.org/drivers/vector/csw.html           |
| VDV            | VDV-451/VDV-452/INTREST Data Format                  | true       | false    | true     | https://gdal.org/drivers/vector/vdv.html           |
| MVT            | Mapbox Vector Tiles                                  | true       | false    | true     | https://gdal.org/drivers/vector/mvt.html           |
| NGW            | NextGIS Web                                          | true       | true     | true     | https://gdal.org/drivers/vector/ngw.html           |
| MapML          | MapML                                                | true       | false    | true     | https://gdal.org/drivers/vector/mapml.html         |
| TIGER          | U.S. Census TIGER/Line                               | false      | false    | true     | https://gdal.org/drivers/vector/tiger.html         |
| AVCBin         | Arc/Info Binary Coverage                             | false      | false    | true     | https://gdal.org/drivers/vector/avcbin.html        |
| AVCE00         | Arc/Info E00 (ASCII) Coverage                        | false      | false    | true     | https://gdal.org/drivers/vector/avce00.html        |

</details>

Note that far from all of these formats have been tested properly, if you run into any issues please first [consult the GDAL docs](https://gdal.org/drivers/vector/index.html), or open an issue here on GitHub.


`ST_Read` also supports limited support for predicate pushdown and spatial filtering (if the underlying GDAL driver supports it), but column pruning (projection pushdown) while technically feasible is not yet implemented.
`ST_Read` also allows using GDAL's virtual filesystem abstractions to read data from remote sources such as S3, or from compressed archives such as zip files.

**Note**: This functionality does not make full use of parallelism due to GDAL not being thread-safe, so you should expect this to be slower than using e.g. the DuckDB Parquet extension to read the same GeoParquet or DuckDBs native csv reader to read csv files. Once we implement support for reading more vector formats natively through this extension (e.g. GeoJSON, GeoBuf, ShapeFile) we will probably split this entire GDAL part into a separate extension.

## Additions of this fork

### KNN join
`ST_KNN(a, b, k [, partition])` is a marker function that only the optimizer understands: it replaces the join with a `SPATIAL_KNN_JOIN` operator. The build side is indexed in a flat R-tree (one per partition value). For each probe row the tree yields candidates by increasing bounding-box distance, a lower bound of the real distance; candidates are refined with the exact geometry distance and the search stops when the next lower bound cannot beat the current k-th best. The result is therefore exact for any geometry type, not only points.

### Point in polygon
A point-versus-polygon predicate never goes through GEOS. `ST_Intersects`, `ST_Contains`, `ST_Within`, `ST_Covers` and `ST_CoveredBy` locate the point with a ray crossing count over the serialized polygon, using the exact orientation predicate, and derive their answer from the location (interior, boundary or exterior). Inside a spatial join, the serialized polygons of the build side do not move for as long as the join runs, so each thread keeps a y-interval index of the rings it has tested, keyed by the address of the polygon; a test then only reads the edges level with the point. Outside of a join nothing can be cached and the polygon is scanned linearly.

### Parallel overlays
DuckDB parallelizes over the rows of the scanned tables, in units of about 120 000 rows, so a table of a few thousand large polygons is processed by one thread. `ST_Intersection`, `ST_Difference` and `ST_Union` therefore look at the chunk they are given: above 256 KB of input geometry, the rows are computed as tasks of DuckDB's scheduler (each with its own GEOS context) while the calling thread takes part, and the results are copied into the result vector afterwards. Below the threshold the functions run as before.

### Clustering window functions
`ST_ClusterDBSCAN` and `ST_ClusterKMeans` are aggregates with a window callback: the whole partition is clustered once, and the callback hands out the value of the current row. DuckDB does not tell a window callback which row it is evaluating, only its frame, so an optimizer rule gives these functions the frame `ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW`, whose end is the position of the row. DBSCAN finds neighbours through a grid of cells of size `eps`.

### Geography
`GEOG` is a `BLOB` holding WKB under its own type name, with longitude/latitude coordinates on WGS84 and geodesic edges. It is deliberately not a `GEOMETRY`: there is no implicit cast, so planar functions do not apply to it by accident, and it keeps its type in databases of any storage version. Areas, lengths and point distances use GeographicLib. The distance to an edge is solved on the ellipsoid by iterating the spherical solution along the geodesic; pairs of edges are pruned with a chord-distance bound; point-in-polygon counts meridian crossings, which handles rings around a pole and across the antimeridian.

### Raster
`RASTER` is a `BLOB` holding an uncompressed GeoTIFF, opened by GDAL from memory without a copy. Functions are thin wrappers over GDAL (warp, DEM processing, rasterize, polygonize) plus native code for pixel access, statistics and map algebra. `ST_MapAlgebra` binds its expression with DuckDB's own binder against a synthetic pixel row and evaluates it with the vectorized expression executor, so any scalar SQL expression can be used.

### Topology
A topology is a schema with `node`, `edge_data` and `face` tables, as in PostGIS. The functions are table functions that run once per call and edit those tables through a separate connection, in a transaction of their own: DuckDB does not let a function modify tables in the statement that calls it. The computation happens in C++ on the rows around the edit.

### Routing
The `pgr_*` functions are table in-out functions: the edge table is a table-valued argument that streams into the function, which builds a compressed adjacency structure and runs the algorithm when the input ends. Since a table in-out function is flushed at the end of every pipeline that feeds it, an optimizer rule places a sort under the function so that the whole input arrives through a single pipeline.

### Overloads and untyped arguments
Several names are shared by the geometry, geography and raster versions of a function. Casting an untyped `NULL` or a string literal to `GEOG` or `RASTER` is registered with a higher cost than casting it to `GEOMETRY`, so calls written before these types existed keep resolving to the geometry functions.
