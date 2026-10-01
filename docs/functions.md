# DuckDB Spatial Function Reference

## Function Index 
**[Scalar Functions](#scalar-functions)**

| Function | Summary |
| --- | --- |
| [`DuckDB_PROJ_Compiled_Version`](#duckdb_proj_compiled_version) | Returns a text description of the PROJ library version that this instance of DuckDB was compiled against. |
| [`DuckDB_Proj_Version`](#duckdb_proj_version) | Returns a text description of the PROJ library version that is being used by this instance of DuckDB. |
| [`ST_3DDistance`](#st_3ddistance) | Returns the 3D Euclidean distance between two POINT geometries. Non-point inputs are rejected (not yet implemented for lines/polygons). |
| [`ST_3DLength`](#st_3dlength) | Returns the 3D length of a linestring (considers Z coordinate) |
| [`ST_3DLineInterpolatePoint`](#st_3dlineinterpolatepoint) | Interpolates a point along a linestring at a fraction of its 3D length |
| [`ST_3DPerimeter`](#st_3dperimeter) | Returns the 3D perimeter of a polygon (considers Z coordinate) |
| [`ST_AddBand`](#st_addband) | Adds a band to a raster and returns the new raster. |
| [`ST_AddMeasure`](#st_addmeasure) | Adds M values along a linestring, interpolated between start and end measures |
| [`ST_AddPoint`](#st_addpoint) | Adds a point to a linestring at a given position (default: end) |
| [`ST_Affine`](#st_affine) | Applies an affine transformation to a geometry. |
| [`ST_Angle`](#st_angle) | Returns the angle in radians between two points |
| [`ST_Area`](#st_area) | Compute the area of a geometry. |
| [`ST_Area_Spheroid`](#st_area_spheroid) | Returns the area of a geometry in meters, using an ellipsoidal model of the earth |
| [`ST_AsEncodedPolyline`](#st_asencodedpolyline) | Encodes a linestring as a Google Encoded Polyline string |
| [`ST_AsEWKB`](#st_asewkb) | Returns the geometry as EWKB (Extended Well-Known Binary). Alias for ST_AsWKB. |
| [`ST_AsEWKT`](#st_asewkt) | Returns the geometry as an Extended WKT (EWKT) string |
| [`ST_AsGDALRaster`](#st_asgdalraster) | Returns the raster as the bytes of a file in a GDAL raster format. |
| [`ST_AsGeoJSON`](#st_asgeojson) | Returns the geometry as a GeoJSON fragment |
| [`ST_AsGML`](#st_asgml) | Returns the geometry as a GML (Geography Markup Language) element. |
| [`ST_AsHEXWKB`](#st_ashexwkb) | Returns the geometry as a HEXWKB string |
| [`ST_AsKML`](#st_askml) | Returns the geometry as a KML (Keyhole Markup Language) geometry element. |
| [`ST_AsLatLonText`](#st_aslatlontext) | Returns a point as a DMS (degrees-minutes-seconds) latitude/longitude string |
| [`ST_AsMVTGeom`](#st_asmvtgeom) | Transform and clip geometry to a tile boundary |
| [`ST_Aspect`](#st_aspect) | Returns the aspect of an elevation band: the compass direction the slope faces, measured clockwise from north (0 = north, 90 = east, 180 = south, 270 = west). Flat pixels are -1. |
| [`ST_AsRaster`](#st_asraster) | Rasterizes a geometry: returns a single-band raster that covers the bounding box of the geometry, where the pixels covered by the geometry have `value` (default 1) and the others are NODATA. |
| [`ST_AsSVG`](#st_assvg) | Convert the geometry into a SVG fragment or path |
| [`ST_AsText`](#st_astext) | Returns the Well-Known Text (WKT) representation of the geometry |
| [`ST_AsTIFF`](#st_astiff) | Returns the raster as the bytes of a GeoTIFF file. |
| [`ST_AsTWKB`](#st_astwkb) | Encodes geometry as Tiny WKB (TWKB) with specified coordinate precision |
| [`ST_AsWKB`](#st_aswkb) | Returns the Well-Known Binary (WKB) representation of the geometry |
| [`ST_Azimuth`](#st_azimuth) | Returns the azimuth (a clockwise angle measured from north) of two points in radian. |
| [`ST_Band`](#st_band) | Returns a raster made of one or more bands of the input raster. |
| [`ST_BandIsNoData`](#st_bandisnodata) | Returns true if every pixel of the band (1-based, default 1) is NODATA. The pixels are always inspected, so `forcechecking` is accepted for compatibility and ignored. |
| [`ST_BandMetaData`](#st_bandmetadata) | Returns the pixel type and NODATA value (NULL if none) of a band (1-based, default 1) as a struct. `isoutdb` is always false and `path` always NULL: bands are always stored in the raster value. |
| [`ST_BandNoDataValue`](#st_bandnodatavalue) | Returns the NODATA value of a band (1-based, default 1), or NULL if the band has none. |
| [`ST_BandPixelType`](#st_bandpixeltype) | Returns the pixel type of a band (1-based, default 1): `8BUI`, `8BSI`, `16BUI`, `16BSI`, `32BUI`, `32BSI`, `32BF` or `64BF`. |
| [`ST_Boundary`](#st_boundary) | Returns the "boundary" of a geometry |
| [`ST_BoundingDiagonal`](#st_boundingdiagonal) | Returns the diagonal of the bounding box as a linestring |
| [`ST_Box2dFromGeoHash`](#st_box2dfromgeohash) | Returns the bounding box polygon of a GeoHash cell |
| [`ST_Buffer`](#st_buffer) | Returns a buffer around the input geometry at the target distance |
| [`ST_BuildArea`](#st_buildarea) | Creates a polygonal geometry by attempting to "fill in" the input geometry. |
| [`ST_Centroid`](#st_centroid) | Returns the centroid of a geometry |
| [`ST_ChaikinSmoothing`](#st_chaikinsmoothing) | Smooths a geometry using Chaikin's corner-cutting algorithm |
| [`ST_Clip`](#st_clip) | Returns the raster clipped by a geometry: pixels outside of the geometry become NODATA. |
| [`ST_ClipByBox2D`](#st_clipbybox2d) | Clips a geometry by a bounding box |
| [`ST_ClosestPoint`](#st_closestpoint) | Returns the closest point on the first geometry to the second geometry |
| [`ST_Collect`](#st_collect) | Collects a list of geometries into a collection geometry. |
| [`ST_CollectionExtract`](#st_collectionextract) | Extracts geometries from a GeometryCollection into a typed multi geometry. |
| [`ST_ColorMap`](#st_colormap) | Returns a raster of up to four `8BUI` bands (grey, RGB or RGBA) that renders band `nband` (1-based, default 1) with a colormap. |
| [`ST_ConcaveHull`](#st_concavehull) | Returns the 'concave' hull of the input geometry, containing all of the source input's points, and which can be used to create polygons from points. The ratio parameter dictates the level of concavity; 1.0 returns the convex hull; and 0 indicates to return the most concave hull possible. Set allowHoles to a non-zero value to allow output containing holes. |
| [`ST_Contains`](#st_contains) | Returns true if the first geometry contains the second geometry |
| [`ST_ContainsProperly`](#st_containsproperly) | Returns true if the first geometry \"properly\" contains the second geometry |
| [`ST_Contour`](#st_contour) | Returns the contour lines of a band as a list of structs `(geom, id, value)`. PostGIS returns a set of rows: use `UNNEST(..., recursive := true)` to get the same shape. |
| [`ST_ConvexHull`](#st_convexhull) | Returns the convex hull enclosing the geometry |
| [`ST_CoordDim`](#st_coorddim) | Returns the coordinate dimension of a geometry |
| [`ST_Count`](#st_count) | Returns the number of pixels of a band (1-based, default 1) that are not NODATA, or the number of all pixels if `exclude_nodata_value` is false. |
| [`ST_CoverageClean`](#st_coverageclean) | Aligns the edges of a list of polygons whose edges are meant to align but are in fact exact matches. |
| [`ST_CoverageInvalidEdges`](#st_coverageinvalidedges) | Returns the invalid edges in a polygonal coverage, which are edges that are not shared by two polygons. |
| [`ST_CoverageSimplify`](#st_coveragesimplify) | Simplify the edges in a polygonal coverage, preserving the coverange by ensuring that the there are no seams between the resulting simplified polygons. |
| [`ST_CoverageUnion`](#st_coverageunion) | Union all geometries in a polygonal coverage into a single geometry. |
| [`ST_CoveredBy`](#st_coveredby) | Returns true if geom1 is "covered by" geom2 |
| [`ST_Covers`](#st_covers) | Returns true if the geom1 "covers" geom2 |
| [`ST_Crosses`](#st_crosses) | Returns true if geom1 "crosses" geom2 |
| [`ST_DelaunayTriangles`](#st_delaunaytriangles) | Returns Delaunay triangulation of input geometry vertices |
| [`ST_DFullyWithin`](#st_dfullywithin) | Returns true if every point of geom1 is within the given distance of geom2. Currently restricted to POINT inputs (reduces to ST_DWithin for points). |
| [`ST_Difference`](#st_difference) | Returns the "difference" between two geometries |
| [`ST_Dimension`](#st_dimension) | Returns the "topological dimension" of a geometry. |
| [`ST_Disjoint`](#st_disjoint) | Returns true if the geometries are disjoint |
| [`ST_Distance`](#st_distance) | Returns the planar distance between two geometries |
| [`ST_Distance_GEOS`](#st_distance_geos) | Returns the planar distance between two geometries |
| [`ST_Distance_Sphere`](#st_distance_sphere) | Returns the haversine (great circle) distance between two geometries. |
| [`ST_Distance_Spheroid`](#st_distance_spheroid) | Returns the distance between two geometries in meters using an ellipsoidal model of the earths surface |
| [`ST_Dump`](#st_dump) | Dumps a geometry into a list of sub-geometries and their "path" in the original geometry. |
| [`ST_DumpAsPolygons`](#st_dumpaspolygons) | Vectorizes a band: returns a list of structs `(geom, val)` with one polygon per connected area of pixels that share a value. PostGIS returns a set of rows: use `UNNEST(..., recursive := true)` to get the same shape. |
| [`ST_DumpValues`](#st_dumpvalues) | Returns the pixel values of a band as a list of rows, each a list of values: `result[y][x]` is the pixel at column `x`, row `y` (both 1-based). |
| [`ST_DWithin`](#st_dwithin) | Returns if two geometries are within a target distance of each-other |
| [`ST_DWithin_GEOS`](#st_dwithin_geos) | Returns true if two geometries are within a target distance of each-other |
| [`ST_DWithin_Spheroid`](#st_dwithin_spheroid) | Returns if two POINT_2D's are within a target distance in meters, using an ellipsoidal model of the earths surface |
| [`ST_EndPoint`](#st_endpoint) | Returns the end point of a LINESTRING. |
| [`ST_Envelope`](#st_envelope) | Returns the minimum bounding rectangle of a geometry as a polygon geometry |
| [`ST_Equals`](#st_equals) | Returns true if the geometries are "equal" |
| [`ST_Expand`](#st_expand) | Expand the input geometry by the specified distance, returning a polygon. |
| [`ST_Extent`](#st_extent) | Returns the minimal bounding box enclosing the input geometry |
| [`ST_Extent_Approx`](#st_extent_approx) | Returns the approximate bounding box of a geometry, if available. |
| [`ST_ExteriorRing`](#st_exteriorring) | Returns the exterior ring (shell) of a polygon geometry. |
| [`ST_FlipCoordinates`](#st_flipcoordinates) | Returns a new geometry with the coordinates of the input geometry "flipped" so that x = y and y = x |
| [`ST_Force2D`](#st_force2d) | Forces the vertices of a geometry to have X and Y components |
| [`ST_Force3DM`](#st_force3dm) | Forces the vertices of a geometry to have X, Y and M components |
| [`ST_Force3DZ`](#st_force3dz) | Forces the vertices of a geometry to have X, Y and Z components |
| [`ST_Force4D`](#st_force4d) | Forces the vertices of a geometry to have X, Y, Z and M components |
| [`ST_ForceCollection`](#st_forcecollection) | Wraps a geometry in a GeometryCollection (no-op if already a collection) |
| [`ST_ForcePolygonCCW`](#st_forcepolygonccw) | Forces polygon exterior rings to be counter-clockwise |
| [`ST_ForcePolygonCW`](#st_forcepolygoncw) | Forces polygon exterior rings to be clockwise |
| [`ST_FrechetDistance`](#st_frechetdistance) | Returns the Frechet distance between two geometries |
| [`ST_FromGDALRaster`](#st_fromgdalraster) | Creates a raster from the bytes of a raster file in any format the bundled GDAL reads from memory (GeoTIFF and Erdas Imagine; see `ST_GDALDrivers()`). |
| [`ST_GeogFromText`](#st_geogfromtext) | Creates a GEOG from its WKT representation. |
| [`ST_GeogFromWKB`](#st_geogfromwkb) | Creates a GEOG from its WKB representation. |
| [`ST_GeogFromWKT`](#st_geogfromwkt) | Creates a GEOG from its WKT representation. |
| [`ST_GeogPoint`](#st_geogpoint) | Creates a GEOG point from a longitude and a latitude in degrees on WGS84 |
| [`ST_GeographyFromText`](#st_geographyfromtext) | Creates a GEOG from its WKT representation. |
| [`ST_GeoHash`](#st_geohash) | Returns the GeoHash string of a geometry's centroid at the given precision |
| [`ST_GeometricMedian`](#st_geometricmedian) | Returns the geometric median of a geometry's vertices (Weiszfeld algorithm) |
| [`ST_GeometryN`](#st_geometryn) | Returns the n-th geometry of a collection or multi-geometry, counting from 1 as in PostGIS and like `ST_PointN` and `ST_InteriorRingN`. |
| [`ST_GeometryType`](#st_geometrytype) | Returns a 'GEOMETRY_TYPE' enum identifying the input geometry type. Possible enum return types are: `POINT`, `LINESTRING`, `POLYGON`, `MULTIPOINT`, `MULTILINESTRING`, `MULTIPOLYGON`, and `GEOMETRYCOLLECTION`. |
| [`ST_GeomFromEWKB`](#st_geomfromewkb) | Creates a geometry from EWKB (Extended Well-Known Binary) data |
| [`ST_GeomFromEWKT`](#st_geomfromewkt) | Parses an Extended WKT (EWKT) string, optionally with SRID prefix |
| [`ST_GeomFromGeoHash`](#st_geomfromgeohash) | Returns the center point of a GeoHash cell |
| [`ST_GeomFromGeoJSON`](#st_geomfromgeojson) | Deserializes a GEOMETRY from a GeoJSON fragment. |
| [`ST_GeomFromGML`](#st_geomfromgml) | Creates a geometry from a GML (Geography Markup Language) geometry element. |
| [`ST_GeomFromHEXEWKB`](#st_geomfromhexewkb) | Deserialize a GEOMETRY from a HEX(E)WKB encoded string |
| [`ST_GeomFromHEXWKB`](#st_geomfromhexwkb) | Deserialize a GEOMETRY from a HEX(E)WKB encoded string |
| [`ST_GeomFromKML`](#st_geomfromkml) | Creates a geometry from a KML (Keyhole Markup Language) geometry element. |
| [`ST_GeomFromText`](#st_geomfromtext) | Deserialize a GEOMETRY from a WKT encoded string |
| [`ST_GeomFromTWKB`](#st_geomfromtwkb) | Decodes a Tiny WKB (TWKB) binary into a geometry |
| [`ST_GeomFromWKB`](#st_geomfromwkb) | Creates a geometry from Well-Known Binary (WKB) representation |
| [`ST_GeoReference`](#st_georeference) | Returns the georeference as the six lines of a world file: `scalex`, `skewy`, `skewx`, `scaley`, `upperleftx`, `upperlefty`, each with 10 decimals and followed by a newline. |
| [`ST_Grayscale`](#st_grayscale) | Returns a single-band `8BUI` raster with the luminance of three bands holding red, green and blue values between 0 and 255: `0.2989 * R + 0.5870 * G + 0.1140 * B`. |
| [`ST_HasM`](#st_hasm) | Check if the input geometry has M values. |
| [`ST_HasNoBand`](#st_hasnoband) | Returns true if the raster has no band with the given number (1-based, default 1). |
| [`ST_HasZ`](#st_hasz) | Check if the input geometry has Z values. |
| [`ST_HausdorffDistance`](#st_hausdorffdistance) | Returns the Hausdorff distance between two geometries |
| [`ST_Height`](#st_height) | Returns the height of the raster in pixels. |
| [`ST_Hilbert`](#st_hilbert) | Encodes the X and Y values as the hilbert curve index for a curve covering the given bounding box. |
| [`ST_Hillshade`](#st_hillshade) | Returns the hypothetical illumination of an elevation band. |
| [`ST_Histogram`](#st_histogram) | Returns the distribution of the pixel values of a band as a list of bins `(min, max, count, percent)`, where `percent` is the fraction of the counted pixels that fall in the bin. PostGIS returns a set of rows: use `UNNEST` to get the same shape. |
| [`ST_InteriorRingN`](#st_interiorringn) | Returns the N-th interior ring (hole) of a POLYGON as a LINESTRING. Indexing is 1-based  (n = 1 returns the first interior ring). Returns NULL if the polygon is empty or has fewer than N interior rings. |
| [`ST_InterpolatePoint`](#st_interpolatepoint) | Computes the closest point on a LINESTRING to a given POINT and returns the interpolated M value of that point. |
| [`ST_InterpolateRaster`](#st_interpolateraster) | Interpolates a surface from the 3D points of a geometry onto the grid of a raster, and returns the raster with band `bandnumber` (1-based, default 1) replaced by the interpolated values. |
| [`ST_Intersection`](#st_intersection) | Returns the intersection of two geometries |
| [`ST_Intersects`](#st_intersects) | Returns true if two geometries intersect |
| [`ST_Intersects_Extent`](#st_intersects_extent) | Returns true if the extent of two geometries intersects |
| [`ST_IsClosed`](#st_isclosed) | Check if a geometry is 'closed' |
| [`ST_IsCollection`](#st_iscollection) | Returns true if geometry is a Multi* or GeometryCollection type |
| [`ST_IsEmpty`](#st_isempty) | Returns true if the geometry is "empty". |
| [`ST_IsPolygonCCW`](#st_ispolygonccw) | Returns true if the exterior ring of a polygon is counter-clockwise |
| [`ST_IsPolygonCW`](#st_ispolygoncw) | Returns true if the exterior ring of a polygon is clockwise |
| [`ST_IsRing`](#st_isring) | Returns true if the geometry is a ring (both ST_IsClosed and ST_IsSimple). |
| [`ST_IsSimple`](#st_issimple) | Returns true if the geometry is simple |
| [`ST_IsValid`](#st_isvalid) | Returns true if the geometry is valid |
| [`ST_IsValidDetail`](#st_isvaliddetail) | Returns a struct with validity info: {valid, reason, location} |
| [`ST_IsValidReason`](#st_isvalidreason) | Returns text explaining why a geometry is invalid, or 'Valid Geometry' |
| [`ST_KNN`](#st_knn) | K-nearest-neighbor join predicate: matches each row of the `geom1` side with its `k` nearest rows of the `geom2` side. |
| [`ST_LargestEmptyCircle`](#st_largestemptycircle) | Returns the largest empty circle within a geometry |
| [`ST_Length`](#st_length) | Returns the length of the input line geometry |
| [`ST_Length_Spheroid`](#st_length_spheroid) | Returns the length of the input geometry in meters, using an ellipsoidal model of the earth |
| [`ST_LineFromEncodedPolyline`](#st_linefromencodedpolyline) | Decodes a Google Encoded Polyline string into a linestring |
| [`ST_LineFromMultiPoint`](#st_linefrommultipoint) | Creates a linestring from the points of a multipoint geometry |
| [`ST_LineInterpolatePoint`](#st_lineinterpolatepoint) | Returns a point interpolated along a line at a fraction of total 2D length. |
| [`ST_LineInterpolatePoints`](#st_lineinterpolatepoints) | Returns a multi-point interpolated along a line at a fraction of total 2D length. |
| [`ST_LineLocatePoint`](#st_linelocatepoint) | Returns the location on a line closest to a point as a fraction of the total 2D length of the line. |
| [`ST_LineMerge`](#st_linemerge) | "Merges" the input line geometry, optionally taking direction into account. |
| [`ST_LineString2DFromWKB`](#st_linestring2dfromwkb) | Deserialize a LINESTRING_2D from a WKB encoded blob |
| [`ST_LineSubstring`](#st_linesubstring) | Returns a substring of a line between two fractions of total 2D length. |
| [`ST_LocateAlong`](#st_locatealong) | Returns a point or multi-point, containing the point(s) at the geometry with the given measure |
| [`ST_LocateBetween`](#st_locatebetween) | Returns a geometry or geometry collection created by filtering and interpolating vertices within a range of "M" values |
| [`ST_LongestLine`](#st_longestline) | Returns the longest line between two geometries (vertex-to-vertex) |
| [`ST_M`](#st_m) | Returns the M coordinate of a point geometry |
| [`ST_MakeBox2D`](#st_makebox2d) | Create a BOX2D from two POINT geometries |
| [`ST_MakeEmptyRaster`](#st_makeemptyraster) | Creates a raster without bands. |
| [`ST_MakeEnvelope`](#st_makeenvelope) | Create a rectangular polygon from min/max coordinates |
| [`ST_MakeLine`](#st_makeline) | Create a LINESTRING from a list of POINT geometries |
| [`ST_MakePoint`](#st_makepoint) | Creates a GEOMETRY point from an pair of floating point numbers. |
| [`ST_MakePolygon`](#st_makepolygon) | Create a POLYGON from a LINESTRING shell |
| [`ST_MakeValid`](#st_makevalid) | Returns a valid representation of the geometry |
| [`ST_MapAlgebra`](#st_mapalgebra) | Computes a new single-band raster pixel by pixel from one band, or from one band of each of two rasters, with a SQL expression. |
| [`ST_MaxDistance`](#st_maxdistance) | Returns the maximum distance between two geometries |
| [`ST_MaximumInscribedCircle`](#st_maximuminscribedcircle) | Returns the maximum inscribed circle of the input geometry, optionally with a tolerance. |
| [`ST_MemSize`](#st_memsize) | Returns the memory size of a geometry in bytes |
| [`ST_MetaData`](#st_metadata) | Returns the size, georeference, SRID (0 when the coordinate system is not an EPSG code) and number of bands of the raster as a struct. |
| [`ST_MinConvexHull`](#st_minconvexhull) | Returns the outline of the smallest pixel window that contains every pixel that is not NODATA, in band `nband` (1-based) or in any band when omitted or NULL. Returns NULL if all pixels are NODATA. |
| [`ST_MinimumBoundingCircle`](#st_minimumboundingcircle) | Returns the minimum bounding circle of a geometry |
| [`ST_MinimumClearance`](#st_minimumclearance) | Returns the minimum clearance of a geometry |
| [`ST_MinimumClearanceLine`](#st_minimumclearanceline) | Returns the line spanning the minimum clearance |
| [`ST_MinimumRotatedRectangle`](#st_minimumrotatedrectangle) | Returns the minimum rotated rectangle that bounds the input geometry, finding the surrounding box that has the lowest area by using a rotated rectangle, rather than taking the lowest and highest coordinate values as per ST_Envelope(). |
| [`ST_MMax`](#st_mmax) | Returns the maximum M coordinate of a geometry |
| [`ST_MMin`](#st_mmin) | Returns the minimum M coordinate of a geometry |
| [`ST_Multi`](#st_multi) | Turns a single geometry into a multi geometry. |
| [`ST_NDims`](#st_ndims) | Returns the topological dimension of a geometry |
| [`ST_NearestValue`](#st_nearestvalue) | Returns the value of the pixel at a point or at a 1-based column and row if it is not NODATA, otherwise the value of the nearest pixel that is not NODATA. |
| [`ST_NGeometries`](#st_ngeometries) | Returns the number of component geometries in a collection geometry. |
| [`ST_NInteriorRings`](#st_ninteriorrings) | Returns the number of interior rings of a polygon |
| [`ST_Node`](#st_node) | Returns a "noded" MultiLinestring, produced by combining a collection of input linestrings and adding additional vertices where they intersect. |
| [`ST_Normalize`](#st_normalize) | Returns the "normalized" representation of the geometry |
| [`ST_NotSameAlignmentReason`](#st_notsamealignmentreason) | Returns the reason why two rasters are not aligned (see ST_SameAlignment), or 'The rasters are aligned'. |
| [`ST_NPoints`](#st_npoints) | Returns the number of vertices within a geometry |
| [`ST_NRings`](#st_nrings) | Returns the number of rings in a polygon (exterior + interior) |
| [`ST_NumBands`](#st_numbands) | Returns the number of bands of the raster. |
| [`ST_NumGeometries`](#st_numgeometries) | Returns the number of component geometries in a collection geometry. |
| [`ST_NumInteriorRings`](#st_numinteriorrings) | Returns the number of interior rings of a polygon |
| [`ST_NumPoints`](#st_numpoints) | Returns the number of vertices within a geometry |
| [`ST_OffsetCurve`](#st_offsetcurve) | Returns an offset curve from a linestring |
| [`ST_OrderingEquals`](#st_orderingequals) | Returns true if two geometries are exactly equal (same vertex order) |
| [`ST_Overlaps`](#st_overlaps) | Returns true if the geometries overlap |
| [`ST_Perimeter`](#st_perimeter) | Returns the length of the perimeter of the geometry |
| [`ST_Perimeter_Spheroid`](#st_perimeter_spheroid) | Returns the length of the perimeter in meters using an ellipsoidal model of the earths surface |
| [`ST_PixelAsCentroid`](#st_pixelascentroid) | Returns the centre of a pixel (1-based column `x` and row `y`) as a point. |
| [`ST_PixelAsCentroids`](#st_pixelascentroids) | Returns one entry per pixel of a band as a list of structs `(geom, val, x, y)`: the pixel's centre as a point, its value and its 1-based column and row. |
| [`ST_PixelAsPoint`](#st_pixelaspoint) | Returns the upper-left corner of a pixel (1-based column `x` and row `y`) as a point. |
| [`ST_PixelAsPoints`](#st_pixelaspoints) | Returns one entry per pixel of a band as a list of structs `(geom, val, x, y)`: the pixel's upper-left corner as a point, its value and its 1-based column and row. |
| [`ST_PixelAsPolygon`](#st_pixelaspolygon) | Returns the outline of a pixel (1-based column `x` and row `y`) as a polygon. |
| [`ST_PixelAsPolygons`](#st_pixelaspolygons) | Returns one entry per pixel of a band as a list of structs `(geom, val, x, y)`: the pixel's outline as a polygon, its value and its 1-based column and row. |
| [`ST_PixelHeight`](#st_pixelheight) | Returns the height of a pixel in world units, taking the skew into account: `sqrt(scaley^2 + skewx^2)`. |
| [`ST_PixelWidth`](#st_pixelwidth) | Returns the width of a pixel in world units, taking the skew into account: `sqrt(scalex^2 + skewy^2)`. |
| [`ST_Point`](#st_point) | Creates a GEOMETRY point |
| [`ST_Point2D`](#st_point2d) | Creates a POINT_2D |
| [`ST_Point2DFromWKB`](#st_point2dfromwkb) | Deserialize a POINT_2D from a WKB encoded blob |
| [`ST_Point3D`](#st_point3d) | Creates a POINT_3D |
| [`ST_Point4D`](#st_point4d) | Creates a POINT_4D |
| [`ST_PointN`](#st_pointn) | Returns the n'th vertex from the input geometry as a point geometry |
| [`ST_PointOnSurface`](#st_pointonsurface) | Returns a point guaranteed to lie on the surface of the geometry |
| [`ST_Points`](#st_points) | Collects all the vertices in the geometry into a MULTIPOINT |
| [`ST_Polygon`](#st_polygon) | Creates a polygon from a closed linestring |
| [`ST_Polygon2DFromWKB`](#st_polygon2dfromwkb) | Deserialize a POLYGON_2D from a WKB encoded blob |
| [`ST_Polygonize`](#st_polygonize) | Returns a polygonized representation of the input geometries |
| [`ST_Project`](#st_project) | Projects a point along the geodesic by a distance (meters) and azimuth (radians) |
| [`ST_QuadKey`](#st_quadkey) | Compute the [quadkey](https://learn.microsoft.com/en-us/bingmaps/articles/bing-maps-tile-system) for a given lon/lat point at a given level. |
| [`ST_Quantile`](#st_quantile) | Returns quantiles of the pixel values of a band. |
| [`ST_QuantizeCoordinates`](#st_quantizecoordinates) | Rounds all coordinates to the given number of decimal places |
| [`ST_RasterToWorldCoord`](#st_rastertoworldcoord) | Returns the world coordinates of the upper-left corner of a pixel as a struct. Pixel columns and rows are numbered from 1 and may lie outside of the raster. |
| [`ST_RasterToWorldCoordX`](#st_rastertoworldcoordx) | Returns the world X coordinate of the upper-left corner of a pixel (columns and rows numbered from 1). The row may be omitted if the raster is not skewed. |
| [`ST_RasterToWorldCoordY`](#st_rastertoworldcoordy) | Returns the world Y coordinate of the upper-left corner of a pixel (columns and rows numbered from 1). The column may be omitted if the raster is not skewed. |
| [`ST_Reclass`](#st_reclass) | Returns the raster with the values of band `nband` (1-based, default 1) mapped to new values, stored with a new pixel type. |
| [`ST_ReducePrecision`](#st_reduceprecision) | Returns the geometry with all vertices reduced to the given precision |
| [`ST_Relate`](#st_relate) | Returns the DE-9IM intersection matrix string |
| [`ST_RelateMatch`](#st_relatematch) | Tests if a DE-9IM matrix string matches a DE-9IM pattern |
| [`ST_RemovePoint`](#st_removepoint) | Removes a point from a linestring (0-indexed, negative from end) |
| [`ST_RemoveRepeatedPoints`](#st_removerepeatedpoints) | Remove repeated points from a LINESTRING. |
| [`ST_Resample`](#st_resample) | Resamples a raster onto another pixel grid that covers the same area, and returns the new raster. |
| [`ST_Rescale`](#st_rescale) | Resamples a raster to a new pixel size, in world units, keeping its extent and upper-left corner. The sign of the scale is ignored: the result is north-up. |
| [`ST_Resize`](#st_resize) | Resamples a raster to a new width and height, keeping its extent. |
| [`ST_Reskew`](#st_reskew) | Resamples a raster onto a grid with the given skew (rotation terms), keeping the pixel size and covering the same extent. |
| [`ST_Reverse`](#st_reverse) | Returns the geometry with the order of its vertices reversed |
| [`ST_Rotation`](#st_rotation) | Returns the rotation of the raster in radians, computed from the pixel column direction (`scalex`, `skewy`). A raster that is not rotated returns 0. |
| [`ST_Roughness`](#st_roughness) | Returns the roughness of an elevation band: the difference between the largest and the smallest value in the 3x3 neighbourhood of each pixel. |
| [`ST_SameAlignment`](#st_samealignment) | Returns true if two rasters have the same coordinate system, scale and skew and their pixel grids line up (a pixel corner of one falls on a pixel corner of the other). The rasters do not need to overlap. |
| [`ST_ScaleX`](#st_scalex) | Returns the X term of the pixel size, in world units per pixel column. |
| [`ST_ScaleY`](#st_scaley) | Returns the Y term of the pixel size, in world units per pixel row (negative for north-up rasters). |
| [`ST_Scroll`](#st_scroll) | Rotates a closed linestring's start point to the vertex nearest to the given point |
| [`ST_Segmentize`](#st_segmentize) | Densifies a geometry by adding vertices so no segment exceeds max_segment_length |
| [`ST_SetBandNoDataValue`](#st_setbandnodatavalue) | Returns the raster with the NODATA value of a band (1-based, default 1) set to `nodatavalue`. NULL removes the NODATA value. The pixel values do not change. `forcechecking` is accepted for compatibility and ignored. |
| [`ST_SetGeoReference`](#st_setgeoreference) | Returns the raster with a new georeference. The pixels are not resampled. |
| [`ST_SetPoint`](#st_setpoint) | Replaces a point in a linestring (0-indexed, negative from end) |
| [`ST_SetScale`](#st_setscale) | Returns the raster with a new pixel size, in world units. The pixels are not resampled (see ST_Rescale). The single-value variant sets both `scalex` and `scaley` to the same value. |
| [`ST_SetSkew`](#st_setskew) | Returns the raster with new skew terms. The pixels are not resampled (see ST_Reskew). The single-value variant sets both `skewx` and `skewy` to the same value. |
| [`ST_SetSRID`](#st_setsrid) | Sets the SRID of a geometry (no-op in DuckDB — use GEOMETRY('EPSG:XXXX') type for CRS) |
| [`ST_SetUpperLeft`](#st_setupperleft) | Returns the raster moved so that its upper-left corner is at the given world coordinates. |
| [`ST_SetValue`](#st_setvalue) | Returns the raster with one pixel (1-based column `x` and row `y`), or every pixel covered by a geometry, set to `newvalue` in a band (1-based, default 1). |
| [`ST_SharedPaths`](#st_sharedpaths) | Returns shared paths between two linear geometries |
| [`ST_ShiftLongitude`](#st_shiftlongitude) | Shifts longitude: negative values get +360, values >180 get -360 |
| [`ST_ShortestLine`](#st_shortestline) | Returns the shortest line between two geometries |
| [`ST_Simplify`](#st_simplify) | Returns a simplified version of the geometry |
| [`ST_SimplifyPolygonHull`](#st_simplifypolygonhull) | Simplifies a polygon while preserving topology |
| [`ST_SimplifyPreserveTopology`](#st_simplifypreservetopology) | Returns a simplified version of the geometry that preserves topology |
| [`ST_SimplifyVW`](#st_simplifyvw) | Simplifies geometry using the Visvalingam-Whyatt area-based algorithm |
| [`ST_SkewX`](#st_skewx) | Returns the X skew of the georeference: the world X offset per pixel row. |
| [`ST_SkewY`](#st_skewy) | Returns the Y skew of the georeference: the world Y offset per pixel column. |
| [`ST_Slope`](#st_slope) | Returns the slope of an elevation band, using Horn's formula. |
| [`ST_Snap`](#st_snap) | Snaps the vertices and segments of a geometry to another geometry's vertices within the given tolerance |
| [`ST_SnapToGrid`](#st_snaptogrid) | Snaps all coordinates to a grid of the given size |
| [`ST_Split`](#st_split) | Splits a geometry by another geometry, returning a geometry collection of the pieces |
| [`ST_SRID`](#st_srid) | Returns the SRID of a geometry (always 0 — DuckDB uses CRS type metadata instead of per-geometry SRIDs) |
| [`ST_StartPoint`](#st_startpoint) | Returns the start point of a LINESTRING. |
| [`ST_Subdivide`](#st_subdivide) | Recursively splits a geometry into sub-geometries until the number of vertices of each are below the threshold given by max_vertices. Accepts any type of input except for a GeometryCollection.Degenerate inputs can lead to results having more than max_vertices vertices due to a recursion depth limit. |
| [`ST_Summary`](#st_summary) | Returns a text summary of a geometry |
| [`ST_SummaryStats`](#st_summarystats) | Returns the count, sum, mean, standard deviation, minimum and maximum of the pixel values of a band, as a struct `(count, sum, mean, stddev, min, max)`. |
| [`ST_SwapOrdinates`](#st_swapordinates) | Swaps two ordinate values in a geometry (e.g., 'xy' swaps x and y) |
| [`ST_SymDifference`](#st_symdifference) | Returns the symmetric difference of two geometries |
| [`ST_Tile`](#st_tile) | Splits a raster into tiles of `width` x `height` pixels and returns them as a list, row by row from the upper-left corner. PostGIS returns a set of rows: use `UNNEST` to get the same shape. |
| [`ST_TileEnvelope`](#st_tileenvelope) | The `ST_TileEnvelope` scalar function generates tile envelope rectangular polygons from specified zoom level and tile indices. |
| [`ST_Touches`](#st_touches) | Returns true if the geometries touch |
| [`ST_TPI`](#st_tpi) | Returns the Topographic Position Index of an elevation band: the value of each pixel minus the mean of its eight neighbours. |
| [`ST_Transform`](#st_transform) | Transforms a geometry between two coordinate systems |
| [`ST_TRI`](#st_tri) | Returns the Terrain Ruggedness Index of an elevation band: the mean absolute difference between each pixel and its eight neighbours. |
| [`ST_TriangulatePolygon`](#st_triangulatepolygon) | Returns constrained Delaunay triangulation of a polygon |
| [`ST_UnaryUnion`](#st_unaryunion) | Dissolves a geometry collection into a single geometry |
| [`ST_Union`](#st_union) | Returns the union of two geometries |
| [`ST_UpperLeftX`](#st_upperleftx) | Returns the world X coordinate of the upper-left corner of the raster. |
| [`ST_UpperLeftY`](#st_upperlefty) | Returns the world Y coordinate of the upper-left corner of the raster. |
| [`ST_Value`](#st_value) | Returns the value of a pixel, addressed by its 1-based column `x` and row `y` or by a point in world coordinates. |
| [`ST_ValueCount`](#st_valuecount) | Counts how often each pixel value occurs in a band. |
| [`ST_VoronoiDiagram`](#st_voronoidiagram) | Returns the Voronoi diagram of the supplied MultiPoint geometry |
| [`ST_Width`](#st_width) | Returns the width of the raster in pixels. |
| [`ST_Within`](#st_within) | Returns true if the first geometry is within the second |
| [`ST_WithinProperly`](#st_withinproperly) | Returns true if the first geometry \"properly\" is contained by the second geometry |
| [`ST_WorldToRasterCoord`](#st_worldtorastercoord) | Returns the 1-based column and row of the pixel that contains a world coordinate or a point, as a struct. The result may lie outside of the raster. |
| [`ST_WorldToRasterCoordX`](#st_worldtorastercoordx) | Returns the 1-based column of the pixel that contains a world coordinate or a point. `yw` may be omitted if the raster is not skewed. |
| [`ST_WorldToRasterCoordY`](#st_worldtorastercoordy) | Returns the 1-based row of the pixel that contains a world coordinate or a point. `xw` may be omitted if the raster is not skewed. |
| [`ST_X`](#st_x) | Returns the X coordinate of a point geometry |
| [`ST_XMax`](#st_xmax) | Returns the maximum X coordinate of a geometry |
| [`ST_XMin`](#st_xmin) | Returns the minimum X coordinate of a geometry |
| [`ST_Y`](#st_y) | Returns the Y coordinate of a point geometry |
| [`ST_YMax`](#st_ymax) | Returns the maximum Y coordinate of a geometry |
| [`ST_YMin`](#st_ymin) | Returns the minimum Y coordinate of a geometry |
| [`ST_Z`](#st_z) | Returns the Z coordinate of a point geometry |
| [`ST_ZMax`](#st_zmax) | Returns the maximum Z coordinate of a geometry |
| [`ST_ZMFlag`](#st_zmflag) | Returns a flag indicating the presence of Z and M values in the input geometry. |
| [`ST_ZMin`](#st_zmin) | Returns the minimum Z coordinate of a geometry |

**[Aggregate Functions](#aggregate-functions)**

| Function | Summary |
| --- | --- |
| [`ST_AsMVT`](#st_asmvt) | Make a Mapbox Vector Tile from a set of geometries and properties |
| [`ST_ClusterDBSCAN`](#st_clusterdbscan) | Assigns a DBSCAN cluster ID to each geometry based on spatial proximity. |
| [`ST_ClusterIntersecting`](#st_clusterintersecting) | Groups intersecting geometries into clusters (returns geometry collection of collections) |
| [`ST_ClusterKMeans`](#st_clusterkmeans) | Assigns a k-means cluster ID to each geometry. |
| [`ST_ClusterWithin`](#st_clusterwithin) | Groups geometries within a given distance into clusters |
| [`ST_CoverageInvalidEdges_Agg`](#st_coverageinvalidedges_agg) | Returns the invalid edges of a coverage geometry |
| [`ST_CoverageSimplify_Agg`](#st_coveragesimplify_agg) | Simplifies a set of geometries while maintaining coverage |
| [`ST_CoverageUnion_Agg`](#st_coverageunion_agg) | Unions a set of geometries while maintaining coverage |
| [`ST_Envelope_Agg`](#st_envelope_agg) | Alias for [ST_Extent_Agg](#st_extent_agg). |
| [`ST_Extent_Agg`](#st_extent_agg) | Computes the minimal-bounding-box polygon containing the set of input geometries |
| [`ST_Intersection_Agg`](#st_intersection_agg) | Computes the intersection of a set of geometries |
| [`ST_MemUnion_Agg`](#st_memunion_agg) | Computes the union of a set of input geometries. |
| [`ST_Retile`](#st_retile) | Aggregate: rebuilds the rasters of a group, a coverage tiled in any way, as a regular set of tiles, and returns the tiles as a list in row order. |
| [`ST_SummaryStatsAgg`](#st_summarystatsagg) | Aggregate: returns the count, sum, mean, population standard deviation, minimum and maximum of the pixel values of a band over all rasters of a group, as a struct `(count, sum, mean, stddev, min, max)`. |
| [`ST_Union_Agg`](#st_union_agg) | Aggregate: merges the rasters of a group into one raster that covers them all. |
| [`TopoElementArray_Agg`](#topoelementarray_agg) | Collects TopoElements into a TopoElementArray. |

**[Macro Functions](#Macro-functions)**

| Function | Summary |
| --- | --- |
| [`ST_Rotate`](#st_rotate) | Alias of ST_RotateZ |
| [`ST_RotateX`](#st_rotatex) | Rotates a geometry around the X axis. This is a shorthand macro for calling ST_Affine. |
| [`ST_RotateY`](#st_rotatey) | Rotates a geometry around the Y axis. This is a shorthand macro for calling ST_Affine. |
| [`ST_RotateZ`](#st_rotatez) | Rotates a geometry around the Z axis. This is a shorthand macro for calling ST_Affine. |
| [`ST_Scale`](#st_scale) |  |
| [`ST_Translate`](#st_translate) |  |
| [`ST_TransScale`](#st_transscale) | Translates and then scales a geometry in X and Y direction. This is a shorthand macro for calling ST_Affine. |

**[Table Functions](#table-functions)**

| Function | Summary |
| --- | --- |
| [`CreateTopology`](#createtopology) | Creates a new, empty topology and returns its id. |
| [`DropTopology`](#droptopology) | Drops a topology: its schema with everything in it, and its row in `topology.topology`. Returns the text `Topology 'name' dropped`. |
| [`GetEdgeByPoint`](#getedgebypoint) | Returns the id of the edge within `tolerance` of a point, or 0 if there is none. |
| [`GetFaceByPoint`](#getfacebypoint) | Returns the id of the face containing a point, or 0 if the point is in the universal face. |
| [`GetNodeByPoint`](#getnodebypoint) | Returns the id of the node within `tolerance` of a point, or 0 if there is none. |
| [`GetNodeEdges`](#getnodeedges) | Returns the edges incident to a node, as rows of `(sequence, edge)` ordered clockwise starting from north. |
| [`GetRingEdges`](#getringedges) | Returns the ordered set of signed edges met by walking along one side of an edge, as rows of `(sequence, edge)`. |
| [`GetTopologyID`](#gettopologyid) | Returns the id of the topology with the given name, or NULL if there is none. |
| [`GetTopologyName`](#gettopologyname) | Returns the name of the topology with the given id, or NULL if there is none. |
| [`GetTopologySRID`](#gettopologysrid) | Returns the SRID the topology with the given name was created with, or NULL if there is no such topology. |
| [`pgr_analyzeGraph`](#pgr_analyzegraph) | Summary of the usual problems of a network topology. |
| [`pgr_aStar`](#pgr_astar) | Shortest path(s) using the A* algorithm. |
| [`pgr_bdAstar`](#pgr_bdastar) | Shortest path(s) using a bidirectional A* search. |
| [`pgr_bdDijkstra`](#pgr_bddijkstra) | Shortest path(s) using a bidirectional Dijkstra search, which grows one search from the start vertex and one from the end vertex. One search is run per pair of start and end vertices. |
| [`pgr_boykovKolmogorov`](#pgr_boykovkolmogorov) | Maximum flow from the source(s) to the sink(s), with the flow carried by each edge. |
| [`pgr_connectedComponents`](#pgr_connectedcomponents) | Connected components of an undirected graph: two vertices are in the same component when a path exists between them, whatever the direction of the edges. |
| [`pgr_createTopology`](#pgr_createtopology) | Builds the topology of a network: gives each edge the identifier of its start and end vertex, snapping end points that are within a tolerance of each other to the same vertex. |
| [`pgr_dijkstra`](#pgr_dijkstra) | Shortest path(s) using Dijkstra's algorithm. |
| [`pgr_dijkstraCost`](#pgr_dijkstracost) | Cost of the shortest path(s) using Dijkstra's algorithm, without the paths themselves. |
| [`pgr_dijkstraCostMatrix`](#pgr_dijkstracostmatrix) | Cost matrix between a set of vertices using Dijkstra's algorithm. The result can be fed to `pgr_TSP`. |
| [`pgr_drivingDistance`](#pgr_drivingdistance) | Vertices whose shortest path cost from the root vertex is less than or equal to a distance, together with the shortest path tree that reaches them. |
| [`pgr_edmondsKarp`](#pgr_edmondskarp) | Maximum flow from the source(s) to the sink(s), with the flow carried by each edge. |
| [`pgr_extractVertices`](#pgr_extractvertices) | Vertices of a graph, extracted from its edges. |
| [`pgr_KSP`](#pgr_ksp) | K shortest loopless paths using Yen's algorithm. |
| [`pgr_maxFlow`](#pgr_maxflow) | Value of the maximum flow from the source(s) to the sink(s), computed with Dinic's algorithm. |
| [`pgr_maxFlowMinCost`](#pgr_maxflowmincost) | Maximum flow of minimum cost from the source(s) to the sink(s): among all the maximum flows, one whose total cost is the smallest. |
| [`pgr_minCostMaxFlow`](#pgr_mincostmaxflow) | Maximum flow of minimum cost from the source(s) to the sink(s): among all the maximum flows, one whose total cost is the smallest. |
| [`pgr_nodeNetwork`](#pgr_nodenetwork) | Nodes a network: splits the lines where they meet, so that lines only touch at their end points. |
| [`pgr_pushRelabel`](#pgr_pushrelabel) | Maximum flow from the source(s) to the sink(s), with the flow carried by each edge. |
| [`pgr_strongComponents`](#pgr_strongcomponents) | Strongly connected components of a directed graph, using Tarjan's algorithm: two vertices are in the same component when each one can be reached from the other. |
| [`pgr_trsp`](#pgr_trsp) | Shortest path(s) with turn restrictions. |
| [`pgr_TSP`](#pgr_tsp) | Travelling salesperson tour over a cost matrix: a round trip that visits every node once. |
| [`pgr_withPoints`](#pgr_withpoints) | Shortest path(s) using Dijkstra's algorithm on a graph to which points located on the edges are added as temporary vertices. |
| [`ST_AddEdgeModFace`](#st_addedgemodface) | Adds an edge between two existing nodes and returns its id. If the edge splits a face, the face is kept for one side and a new face is added for the other. |
| [`ST_AddEdgeNewFaces`](#st_addedgenewfaces) | Adds an edge between two existing nodes and returns its id. If the edge splits a face, the face is deleted and replaced by two new faces. |
| [`ST_AddIsoEdge`](#st_addisoedge) | Adds an isolated edge between two isolated nodes of the same face and returns its id. |
| [`ST_AddIsoNode`](#st_addisonode) | Adds an isolated node to a face of a topology and returns its id. |
| [`ST_ChangeEdgeGeom`](#st_changeedgegeom) | Changes the shape of an edge without changing the structure of the topology. Returns the text `Edge N changed`. |
| [`ST_CreateTopoGeo`](#st_createtopogeo) | Populates an empty topology from a geometry collection and returns the text `Topology name populated`. |
| [`ST_Drivers`](#st_drivers) | Returns the list of supported GDAL drivers and file formats |
| [`ST_DumpPoints`](#st_dumppoints) | Extracts all vertices from a geometry as individual point geometries. |
| [`ST_DumpRings`](#st_dumprings) | Extracts the rings of a polygon geometry. |
| [`ST_DumpSegments`](#st_dumpsegments) | Extracts consecutive vertex pairs from a geometry as 2-point linestring segments. |
| [`ST_GDALDrivers`](#st_gdaldrivers) | Returns the GDAL raster drivers that are built into the extension: the formats that `ST_ReadRaster` and `ST_FromGDALRaster` can read and that `ST_AsGDALRaster` can write. |
| [`ST_GeneratePoints`](#st_generatepoints) | Generates a set of random points within the specified bounding box. |
| [`ST_GetFaceEdges`](#st_getfaceedges) | Returns the ordered set of signed edges bounding a face, as rows of `(sequence, edge)`. |
| [`ST_GetFaceGeometry`](#st_getfacegeometry) | Returns the polygon of a face, built from the edges that have the face on exactly one side. |
| [`ST_HexagonGrid`](#st_hexagongrid) | Generates a regular hexagonal grid covering the bounding box of the input geometry. |
| [`ST_ModEdgeHeal`](#st_modedgeheal) | Heals two edges by deleting the node connecting them, modifying the first edge and deleting the second. Returns the id of the deleted node. |
| [`ST_ModEdgeSplit`](#st_modedgesplit) | Splits an edge by creating a node on it, modifying the original edge and adding a new one. Returns the id of the new node. |
| [`ST_MoveIsoNode`](#st_moveisonode) | Moves an isolated node to another location within its face. Returns the text `Isolated Node N moved to location x,y`. |
| [`ST_NewEdgeHeal`](#st_newedgeheal) | Heals two edges by deleting the node connecting them and replacing both edges with a new one, which has the direction of the first edge. Returns the id of the new edge. |
| [`ST_NewEdgesSplit`](#st_newedgessplit) | Splits an edge by creating a node on it, deleting the original edge and replacing it with two new edges. Returns the id of the new node. |
| [`ST_Read`](#st_read) | Read and import a variety of geospatial file formats using the GDAL library. |
| [`ST_Read_Meta`](#st_read_meta) | Read the metadata from a variety of geospatial file formats using the GDAL library. |
| [`ST_ReadOSM`](#st_readosm) | The `ST_ReadOsm()` table function enables reading compressed OpenStreetMap data directly from a `.osm.pbf` file. |
| [`ST_ReadRaster`](#st_readraster) | Reads a raster file, or every file matching a glob pattern, and returns its pixels as `RASTER` values. |
| [`ST_ReadSHP`](#st_readshp) | Read a Shapefile without relying on the GDAL library |
| [`ST_RemEdgeModFace`](#st_remedgemodface) | Removes an edge. If it separates two faces, one is deleted and the other is modified to cover both. |
| [`ST_RemEdgeNewFace`](#st_remedgenewface) | Removes an edge. If it separates two faces, both are deleted and replaced by a new face covering them. |
| [`ST_RemoveIsoEdge`](#st_removeisoedge) | Removes an isolated edge. Its end nodes become isolated nodes of the face the edge was in. Returns the text `Isolated edge N removed`. |
| [`ST_RemoveIsoNode`](#st_removeisonode) | Removes an isolated node. Returns the text `Isolated node N removed`. |
| [`ST_SquareGrid`](#st_squaregrid) | Generates a regular grid of square polygons covering the bounding box of the input geometry. |
| [`TopologySummary`](#topologysummary) | Returns a two-line text summary of a topology: its id, SRID and precision, then the number of nodes, edges and faces (the universal face is not counted). |
| [`ValidateTopology`](#validatetopology) | Checks a topology and returns one `(error, id1, id2)` row per problem found; a valid topology yields no rows. |

----

## Scalar Functions

### DuckDB_PROJ_Compiled_Version


#### Signature

```sql
VARCHAR DuckDB_PROJ_Compiled_Version ()
```

#### Description

Returns a text description of the PROJ library version that this instance of DuckDB was compiled against.

#### Example

```sql
SELECT duckdb_proj_compiled_version();
┌────────────────────────────────┐
│ duckdb_proj_compiled_version() │
│            varchar             │
├────────────────────────────────┤
│ Rel. 9.1.1, December 1st, 2022 │
└────────────────────────────────┘
```

----

### DuckDB_Proj_Version


#### Signature

```sql
VARCHAR DuckDB_Proj_Version ()
```

#### Description

Returns a text description of the PROJ library version that is being used by this instance of DuckDB.

#### Example

```sql
SELECT duckdb_proj_version();
┌───────────────────────┐
│ duckdb_proj_version() │
│        varchar        │
├───────────────────────┤
│ 9.1.1                 │geometry_always_xy
└───────────────────────┘
```

----

### ST_3DDistance


#### Signature

```sql
DOUBLE ST_3DDistance (geom1 GEOMETRY, geom2 GEOMETRY)
```

#### Description

Returns the 3D Euclidean distance between two POINT geometries. Non-point inputs are rejected (not yet implemented for lines/polygons).

#### Example

```sql
SELECT ST_3DDistance(ST_GeomFromText('POINT Z(0 0 0)'), ST_GeomFromText('POINT Z(1 1 1)'))
```

----

### ST_3DLength


#### Signature

```sql
DOUBLE ST_3DLength (geom GEOMETRY)
```

#### Description

Returns the 3D length of a linestring (considers Z coordinate)

#### Example

```sql
SELECT ST_3DLength(ST_GeomFromText('LINESTRING Z(0 0 0, 1 0 0, 1 1 1)'))
```

----

### ST_3DLineInterpolatePoint


#### Signature

```sql
GEOMETRY ST_3DLineInterpolatePoint (line GEOMETRY, fraction DOUBLE)
```

#### Description

Interpolates a point along a linestring at a fraction of its 3D length

#### Example

```sql
SELECT ST_AsText(ST_3DLineInterpolatePoint(ST_GeomFromText('LINESTRING Z(0 0 0, 10 0 10)'), 0.5))
```

----

### ST_3DPerimeter


#### Signature

```sql
DOUBLE ST_3DPerimeter (geom GEOMETRY)
```

#### Description

Returns the 3D perimeter of a polygon (considers Z coordinate)

#### Example

```sql
SELECT ST_3DPerimeter(ST_GeomFromText('POLYGON Z((0 0 0, 1 0 0, 1 1 1, 0 1 0, 0 0 0))'))
```

----

### ST_AddBand


#### Signatures

```sql
RASTER ST_AddBand (rast RASTER, pixeltype VARCHAR)
RASTER ST_AddBand (rast RASTER, pixeltype VARCHAR, initialvalue DOUBLE)
RASTER ST_AddBand (rast RASTER, pixeltype VARCHAR, initialvalue DOUBLE, nodataval DOUBLE)
RASTER ST_AddBand (rast RASTER, index INTEGER, pixeltype VARCHAR)
RASTER ST_AddBand (rast RASTER, index INTEGER, pixeltype VARCHAR, initialvalue DOUBLE)
RASTER ST_AddBand (rast RASTER, index INTEGER, pixeltype VARCHAR, initialvalue DOUBLE, nodataval DOUBLE)
RASTER ST_AddBand (torast RASTER, fromrast RASTER)
RASTER ST_AddBand (torast RASTER, fromrast RASTER, fromband INTEGER)
RASTER ST_AddBand (torast RASTER, fromrast RASTER, fromband INTEGER, torastindex INTEGER)
RASTER ST_AddBand (torast RASTER, fromrasts RASTER[])
RASTER ST_AddBand (torast RASTER, fromrasts RASTER[], fromband INTEGER)
RASTER ST_AddBand (torast RASTER, fromrasts RASTER[], fromband INTEGER, torastindex INTEGER)
```

#### Description

Adds a band to a raster and returns the new raster.

`pixeltype` is one of `8BUI`, `8BSI`, `16BUI`, `16BSI`, `32BUI`, `32BSI`, `32BF`, `64BF`. The band is filled with `initialvalue` (default 0) and gets `nodataval` as its NODATA value (default: none). `index` is the 1-based position of the new band; by default it is appended.

The `fromrast` variants copy band `fromband` (default 1) of another raster of the same width and height, or of each raster in a list, and insert the copies at `torastindex` (default: at the end).

Differences from PostGIS: all bands of a raster share one pixel type, so adding a band of another type is an error; `1BB`, `2BUI` and `4BUI` are accepted but stored as `8BUI`; out-of-database bands are not supported.

#### Example

```sql
SELECT ST_BandPixelType(ST_AddBand(ST_MakeEmptyRaster(4, 4, 0, 0, 1), '32BF', 1.5, -9999));
----
32BF
```

----

### ST_AddMeasure


#### Signature

```sql
GEOMETRY ST_AddMeasure (line GEOMETRY, measure_start DOUBLE, measure_end DOUBLE)
```

#### Description

Adds M values along a linestring, interpolated between start and end measures

#### Example

```sql
SELECT ST_AsText(ST_AddMeasure(ST_GeomFromText('LINESTRING(0 0, 5 0, 10 0)'), 0, 100))
```

----

### ST_AddPoint


#### Signatures

```sql
GEOMETRY ST_AddPoint (line GEOMETRY, point GEOMETRY)
GEOMETRY ST_AddPoint (line GEOMETRY, point GEOMETRY, position INTEGER)
```

#### Description

Adds a point to a linestring at a given position (default: end)

#### Example

```sql
SELECT ST_AsText(ST_AddPoint(ST_GeomFromText('LINESTRING(0 0, 2 2)'), ST_Point(1, 1), 1))
```

----

### ST_Affine


#### Signatures

```sql
GEOMETRY ST_Affine (geom GEOMETRY, a DOUBLE, b DOUBLE, c DOUBLE, d DOUBLE, e DOUBLE, f DOUBLE, g DOUBLE, h DOUBLE, i DOUBLE, xoff DOUBLE, yoff DOUBLE, zoff DOUBLE)
GEOMETRY ST_Affine (geom GEOMETRY, a DOUBLE, b DOUBLE, d DOUBLE, e DOUBLE, xoff DOUBLE, yoff DOUBLE)
```

#### Description

Applies an affine transformation to a geometry.

For the 2D variant, the transformation matrix is defined as follows:
```
| a b xoff |
| d e yoff |
| 0 0 1    |
```

For the 3D variant, the transformation matrix is defined as follows:
```
| a b c xoff |
| d e f yoff |
| g h i zoff |
| 0 0 0 1    |
```

The transformation is applied to all vertices of the geometry.

#### Example

```sql
-- Translate a point by (2, 3)
SELECT ST_Affine(ST_Point(1, 1),
                 1, 0,   -- a, b
                 0, 1,   -- d, e
                 2, 3);  -- xoff, yoff
----
POINT (3 4)

-- Scale a geometry by factor 2 in X and Y
SELECT ST_Affine(ST_Point(1, 1),
                 2, 0, 0,   -- a, b, c
                 0, 2, 0,   -- d, e, f
                 0, 0, 1,   -- g, h, i
                 0, 0, 0);  -- xoff, yoff, zoff
----
POINT (2 2)
```

----

### ST_Angle


#### Signature

```sql
DOUBLE ST_Angle (point1 GEOMETRY, point2 GEOMETRY)
```

#### Description

Returns the angle in radians between two points

----

### ST_Area


#### Signatures

```sql
DOUBLE ST_Area (geom GEOMETRY)
DOUBLE ST_Area (polygon POLYGON_2D)
DOUBLE ST_Area (linestring LINESTRING_2D)
DOUBLE ST_Area (point POINT_2D)
DOUBLE ST_Area (box BOX_2D)
DOUBLE ST_Area (geog GEOG)
```

#### Description

Compute the area of a geometry.

Returns `0.0` for any geometry that is not a `POLYGON`, `MULTIPOLYGON` or `GEOMETRYCOLLECTION` containing polygon
geometries.

The area is in the same units as the spatial reference system of the geometry.

The `POINT_2D` and `LINESTRING_2D` overloads of this function always return `0.0` but are included for completeness.

#### Example

```sql
SELECT ST_Area('POLYGON((0 0, 0 1, 1 1, 1 0, 0 0))'::GEOMETRY);
-- 1.0
```

----

### ST_Area_Spheroid


#### Signatures

```sql
DOUBLE ST_Area_Spheroid (geom GEOMETRY)
DOUBLE ST_Area_Spheroid (poly POLYGON_2D)
```

#### Description

Returns the area of a geometry in meters, using an ellipsoidal model of the earth

The input geometry is assumed to be in the [EPSG:4326](https://en.wikipedia.org/wiki/World_Geodetic_System) coordinate system (WGS84), with [latitude, longitude] axis order and the area is returned in square meters. This function uses the [GeographicLib](https://geographiclib.sourceforge.io/) library, calculating the area using an ellipsoidal model of the earth. This is a highly accurate method for calculating the area of a polygon taking the curvature of the earth into account, but is also the slowest.

Returns `0.0` for any geometry that is not a `POLYGON`, `MULTIPOLYGON` or `GEOMETRYCOLLECTION` containing polygon geometries.

----

### ST_AsEncodedPolyline


#### Signature

```sql
VARCHAR ST_AsEncodedPolyline (line GEOMETRY)
```

#### Description

Encodes a linestring as a Google Encoded Polyline string

#### Example

```sql
SELECT ST_AsEncodedPolyline(ST_GeomFromText('LINESTRING(-120.2 38.5, -120.95 40.7, -126.453 43.252)'))
```

----

### ST_AsEWKB


#### Signature

```sql
BLOB ST_AsEWKB (geom GEOMETRY)
```

#### Description

Returns the geometry as EWKB (Extended Well-Known Binary). Alias for ST_AsWKB.

#### Example

```sql
SELECT ST_AsEWKB(ST_Point(1, 2))::BLOB
```

----

### ST_AsEWKT


#### Signature

```sql
VARCHAR ST_AsEWKT (geom GEOMETRY)
```

#### Description

Returns the geometry as an Extended WKT (EWKT) string

#### Example

```sql
SELECT ST_AsEWKT(ST_Point(1, 2))
```

----

### ST_AsGDALRaster


#### Signatures

```sql
BLOB ST_AsGDALRaster (rast RASTER, format VARCHAR)
BLOB ST_AsGDALRaster (rast RASTER, format VARCHAR, options VARCHAR[])
BLOB ST_AsGDALRaster (rast RASTER, format VARCHAR, options VARCHAR[], srid INTEGER)
```

#### Description

Returns the raster as the bytes of a file in a GDAL raster format.

`format` is the short name of a driver that can write (see `ST_GDALDrivers()`), `options` is a list of `NAME=VALUE` creation options of that driver, and `srid` overrides the coordinate system written to the file with an EPSG code, without reprojecting. The bundled GDAL has no PNG or JPEG driver. A raster without bands cannot be exported.

#### Example

```sql
SELECT octet_length(ST_AsGDALRaster(ST_AddBand(ST_MakeEmptyRaster(3, 2, 0, 0, 1), '8BUI'), 'GTiff', ['COMPRESS=DEFLATE'])) > 0;
----
true
```

----

### ST_AsGeoJSON


#### Signature

```sql
JSON ST_AsGeoJSON (geom GEOMETRY)
```

#### Description

Returns the geometry as a GeoJSON fragment

This does not return a complete GeoJSON document, only the geometry fragment.
To construct a complete GeoJSON document or feature, look into using the DuckDB JSON extension in conjunction with this function.
This function supports geometries with Z values, but not M values. M values are ignored.

#### Example

```sql
SELECT ST_AsGeoJSON('POLYGON((0 0, 0 1, 1 1, 1 0, 0 0))'::GEOMETRY);
----
{"type":"Polygon","coordinates":[[[0.0, 0.0], [0.0, 1.0], [1.0, 1.0], [1.0, 0.0], [0.0, 0.0]]]}

-- Convert a geometry into a full GeoJSON feature (requires the JSON extension to be loaded)
SELECT CAST({
    type: 'Feature',
    geometry: ST_AsGeoJSON(ST_Point(1, 2)),
    properties: {
        name: 'my_point'
    }
} AS JSON);
----
{"type":"Feature","geometry":{"type":"Point","coordinates":[1.0, 2.0]},"properties":{"name":"my_point"}}
```

----

### ST_AsGML


#### Signatures

```sql
VARCHAR ST_AsGML (geom GEOMETRY)
VARCHAR ST_AsGML (version INTEGER, geom GEOMETRY)
```

#### Description

Returns the geometry as a GML (Geography Markup Language) element.

The `version` is 2 (GML 2.1.2, the default) or 3 (GML 3.1.1). Coordinates are written as they are, with 15 significant digits, and no `srsName` is emitted. M values are dropped, and an empty geometry returns `NULL`.

#### Example

```sql
SELECT ST_AsGML(ST_Point(1, 2));
----
<gml:Point><gml:coordinates>1,2</gml:coordinates></gml:Point>

SELECT ST_AsGML(3, ST_Point(1, 2));
----
<gml:Point><gml:pos>1 2</gml:pos></gml:Point>
```

----

### ST_AsHEXWKB


#### Signature

```sql
VARCHAR ST_AsHEXWKB (geom GEOMETRY)
```

#### Description

Returns the geometry as a HEXWKB string

#### Example

```sql
SELECT ST_AsHexWKB('POLYGON((0 0, 0 1, 1 1, 1 0, 0 0))'::GEOMETRY);
----
01030000000100000005000000000000000000000000000...
```

----

### ST_AsKML


#### Signature

```sql
VARCHAR ST_AsKML (geom GEOMETRY)
```

#### Description

Returns the geometry as a KML (Keyhole Markup Language) geometry element.

KML coordinates are longitude, latitude in WGS84, written with 15 significant digits. The geometry is not reprojected: coordinates outside of the longitude/latitude range raise an error, so transform the geometry to `EPSG:4326` (with `always_xy := true`) first if it is in another coordinate system. M values are dropped, and an empty geometry returns `NULL`.

#### Example

```sql
SELECT ST_AsKML(ST_Point(4.35, 50.85));
----
<Point><coordinates>4.35,50.85</coordinates></Point>
```

----

### ST_AsLatLonText


#### Signature

```sql
VARCHAR ST_AsLatLonText (geom GEOMETRY)
```

#### Description

Returns a point as a DMS (degrees-minutes-seconds) latitude/longitude string

#### Example

```sql
SELECT ST_AsLatLonText(ST_Point(-73.9857, 40.7484))
```

----

### ST_AsMVTGeom


#### Signatures

```sql
GEOMETRY ST_AsMVTGeom (geom GEOMETRY, bounds BOX_2D, extent BIGINT, buffer BIGINT, clip_geom BOOLEAN)
GEOMETRY ST_AsMVTGeom (geom GEOMETRY, bounds BOX_2D, extent BIGINT, buffer BIGINT)
GEOMETRY ST_AsMVTGeom (geom GEOMETRY, bounds BOX_2D, extent BIGINT)
GEOMETRY ST_AsMVTGeom (geom GEOMETRY, bounds BOX_2D)
```

#### Description

Transform and clip geometry to a tile boundary

See "ST_AsMVT" for more details

----

### ST_Aspect


#### Signatures

```sql
RASTER ST_Aspect (rast RASTER)
RASTER ST_Aspect (rast RASTER, nband INTEGER)
RASTER ST_Aspect (rast RASTER, nband INTEGER, pixeltype VARCHAR)
RASTER ST_Aspect (rast RASTER, nband INTEGER, pixeltype VARCHAR, units VARCHAR)
RASTER ST_Aspect (rast RASTER, nband INTEGER, pixeltype VARCHAR, units VARCHAR, interpolate_nodata BOOLEAN)
```

#### Description

Returns the aspect of an elevation band: the compass direction the slope faces, measured clockwise from north (0 = north, 90 = east, 180 = south, 270 = west). Flat pixels are -1.

`units` is `DEGREES` (the default) or `RADIANS`.

Computed by GDAL's DEM processing on the 3x3 neighbourhood of each pixel. Pixels on the border and next to NODATA pixels are computed from a neighbourhood that GDAL extrapolates, where PostGIS substitutes the value of the centre pixel, so those pixels can differ from PostGIS. `nband` is 1-based (default 1) and `pixeltype` the pixel type of the result (default `32BF`). NODATA pixels are NODATA in the result (-9999, or the largest value of the pixel type if it cannot hold -9999). `interpolate_nodata` must be false and the `customextent` variants are not available.

#### Example

```sql
SELECT ST_Value(ST_Aspect(ST_SetValue(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(3, 3, 0, 3, 1), '32BF'), ST_MakeEnvelope(1, 0, 2, 3), 1), ST_MakeEnvelope(2, 0, 3, 3), 2)), 2, 2);
----
270.0
```

----

### ST_AsRaster


#### Signatures

```sql
RASTER ST_AsRaster (geom GEOMETRY, ref RASTER)
RASTER ST_AsRaster (geom GEOMETRY, ref RASTER, pixeltype VARCHAR)
RASTER ST_AsRaster (geom GEOMETRY, ref RASTER, pixeltype VARCHAR, value DOUBLE)
RASTER ST_AsRaster (geom GEOMETRY, ref RASTER, pixeltype VARCHAR, value DOUBLE, nodataval DOUBLE)
RASTER ST_AsRaster (geom GEOMETRY, ref RASTER, pixeltype VARCHAR, value DOUBLE, nodataval DOUBLE, touched BOOLEAN)
RASTER ST_AsRaster (geom GEOMETRY, scalex DOUBLE, scaley DOUBLE, pixeltype VARCHAR)
RASTER ST_AsRaster (geom GEOMETRY, scalex DOUBLE, scaley DOUBLE, pixeltype VARCHAR, value DOUBLE)
RASTER ST_AsRaster (geom GEOMETRY, scalex DOUBLE, scaley DOUBLE, pixeltype VARCHAR, value DOUBLE, nodataval DOUBLE)
RASTER ST_AsRaster (geom GEOMETRY, scalex DOUBLE, scaley DOUBLE, pixeltype VARCHAR, value DOUBLE, nodataval DOUBLE, upperleftx DOUBLE)
RASTER ST_AsRaster (geom GEOMETRY, scalex DOUBLE, scaley DOUBLE, pixeltype VARCHAR, value DOUBLE, nodataval DOUBLE, upperleftx DOUBLE, upperlefty DOUBLE)
RASTER ST_AsRaster (geom GEOMETRY, scalex DOUBLE, scaley DOUBLE, pixeltype VARCHAR, value DOUBLE, nodataval DOUBLE, upperleftx DOUBLE, upperlefty DOUBLE, skewx DOUBLE)
RASTER ST_AsRaster (geom GEOMETRY, scalex DOUBLE, scaley DOUBLE, pixeltype VARCHAR, value DOUBLE, nodataval DOUBLE, upperleftx DOUBLE, upperlefty DOUBLE, skewx DOUBLE, skewy DOUBLE)
RASTER ST_AsRaster (geom GEOMETRY, scalex DOUBLE, scaley DOUBLE, pixeltype VARCHAR, value DOUBLE, nodataval DOUBLE, upperleftx DOUBLE, upperlefty DOUBLE, skewx DOUBLE, skewy DOUBLE, touched BOOLEAN)
RASTER ST_AsRaster (geom GEOMETRY, scalex DOUBLE, scaley DOUBLE, gridx DOUBLE, gridy DOUBLE, pixeltype VARCHAR)
RASTER ST_AsRaster (geom GEOMETRY, scalex DOUBLE, scaley DOUBLE, gridx DOUBLE, gridy DOUBLE, pixeltype VARCHAR, value DOUBLE)
RASTER ST_AsRaster (geom GEOMETRY, scalex DOUBLE, scaley DOUBLE, gridx DOUBLE, gridy DOUBLE, pixeltype VARCHAR, value DOUBLE, nodataval DOUBLE)
RASTER ST_AsRaster (geom GEOMETRY, scalex DOUBLE, scaley DOUBLE, gridx DOUBLE, gridy DOUBLE, pixeltype VARCHAR, value DOUBLE, nodataval DOUBLE, skewx DOUBLE)
RASTER ST_AsRaster (geom GEOMETRY, scalex DOUBLE, scaley DOUBLE, gridx DOUBLE, gridy DOUBLE, pixeltype VARCHAR, value DOUBLE, nodataval DOUBLE, skewx DOUBLE, skewy DOUBLE)
RASTER ST_AsRaster (geom GEOMETRY, scalex DOUBLE, scaley DOUBLE, gridx DOUBLE, gridy DOUBLE, pixeltype VARCHAR, value DOUBLE, nodataval DOUBLE, skewx DOUBLE, skewy DOUBLE, touched BOOLEAN)
RASTER ST_AsRaster (geom GEOMETRY, width INTEGER, height INTEGER, pixeltype VARCHAR)
RASTER ST_AsRaster (geom GEOMETRY, width INTEGER, height INTEGER, pixeltype VARCHAR, value DOUBLE)
RASTER ST_AsRaster (geom GEOMETRY, width INTEGER, height INTEGER, pixeltype VARCHAR, value DOUBLE, nodataval DOUBLE)
RASTER ST_AsRaster (geom GEOMETRY, width INTEGER, height INTEGER, pixeltype VARCHAR, value DOUBLE, nodataval DOUBLE, upperleftx DOUBLE)
RASTER ST_AsRaster (geom GEOMETRY, width INTEGER, height INTEGER, pixeltype VARCHAR, value DOUBLE, nodataval DOUBLE, upperleftx DOUBLE, upperlefty DOUBLE)
RASTER ST_AsRaster (geom GEOMETRY, width INTEGER, height INTEGER, pixeltype VARCHAR, value DOUBLE, nodataval DOUBLE, upperleftx DOUBLE, upperlefty DOUBLE, skewx DOUBLE)
RASTER ST_AsRaster (geom GEOMETRY, width INTEGER, height INTEGER, pixeltype VARCHAR, value DOUBLE, nodataval DOUBLE, upperleftx DOUBLE, upperlefty DOUBLE, skewx DOUBLE, skewy DOUBLE)
RASTER ST_AsRaster (geom GEOMETRY, width INTEGER, height INTEGER, pixeltype VARCHAR, value DOUBLE, nodataval DOUBLE, upperleftx DOUBLE, upperlefty DOUBLE, skewx DOUBLE, skewy DOUBLE, touched BOOLEAN)
RASTER ST_AsRaster (geom GEOMETRY, width INTEGER, height INTEGER, gridx DOUBLE, gridy DOUBLE, pixeltype VARCHAR)
RASTER ST_AsRaster (geom GEOMETRY, width INTEGER, height INTEGER, gridx DOUBLE, gridy DOUBLE, pixeltype VARCHAR, value DOUBLE)
RASTER ST_AsRaster (geom GEOMETRY, width INTEGER, height INTEGER, gridx DOUBLE, gridy DOUBLE, pixeltype VARCHAR, value DOUBLE, nodataval DOUBLE)
RASTER ST_AsRaster (geom GEOMETRY, width INTEGER, height INTEGER, gridx DOUBLE, gridy DOUBLE, pixeltype VARCHAR, value DOUBLE, nodataval DOUBLE, skewx DOUBLE)
RASTER ST_AsRaster (geom GEOMETRY, width INTEGER, height INTEGER, gridx DOUBLE, gridy DOUBLE, pixeltype VARCHAR, value DOUBLE, nodataval DOUBLE, skewx DOUBLE, skewy DOUBLE)
RASTER ST_AsRaster (geom GEOMETRY, width INTEGER, height INTEGER, gridx DOUBLE, gridy DOUBLE, pixeltype VARCHAR, value DOUBLE, nodataval DOUBLE, skewx DOUBLE, skewy DOUBLE, touched BOOLEAN)
```

#### Description

Rasterizes a geometry: returns a single-band raster that covers the bounding box of the geometry, where the pixels covered by the geometry have `value` (default 1) and the others are NODATA.

The grid is that of a reference raster `ref` (same pixel size, skew, alignment and coordinate system), or is given by a pixel size (`scalex`, `scaley`, in world units, sign ignored) or a size in pixels (`width`, `height`). `upperleftx`/`upperlefty` fix the upper-left corner of the result, `gridx`/`gridy` only align its pixel corners with that point; `skewx`/`skewy` default to 0. Without a reference raster the result is north-up and has no coordinate system.

`pixeltype` defaults to `8BUI` and `nodataval` to 0; a NULL `nodataval` gives a band without NODATA value whose background is 0. A polygon covers the pixels whose centre it contains, lines and points the pixels they pass through; with `touched` every pixel the geometry touches is covered. Returns NULL for an empty geometry. The variants of PostGIS that take lists of pixel types, values and NODATA values (several bands) are not available.

#### Example

```sql
SELECT ST_DumpValues(ST_AsRaster(ST_MakeEnvelope(0, 0, 2, 1), 1.0, 1.0, '8BUI', 7));
----
[[7.0, 7.0]]
```

----

### ST_AsSVG


#### Signature

```sql
VARCHAR ST_AsSVG (geom GEOMETRY, relative BOOLEAN, precision INTEGER)
```

#### Description

Convert the geometry into a SVG fragment or path

The SVG fragment is returned as a string. The fragment is a path element that can be used in an SVG document.
The second boolean argument specifies whether the path should be relative or absolute.
The third argument specifies the maximum number of digits to use for the coordinates.

Points are formatted as cx/cy using absolute coordinates or x/y using relative coordinates.

#### Example

```sql
SELECT ST_AsSVG('POLYGON((0 0, 0 1, 1 1, 1 0, 0 0))'::GEOMETRY, false, 15);
----
M 0 0 L 0 -1 1 -1 1 0 Z
```

----

### ST_AsText


#### Signatures

```sql
VARCHAR ST_AsText (geom GEOMETRY)
VARCHAR ST_AsText (point POINT_2D)
VARCHAR ST_AsText (linestring LINESTRING_2D)
VARCHAR ST_AsText (polygon POLYGON_2D)
VARCHAR ST_AsText (box BOX_2D)
VARCHAR ST_AsText (geog GEOG)
```

#### Description

Returns the Well-Known Text (WKT) representation of the geometry

#### Example

```sql
ST_AsText(ST_GeomFromWKB(X'01010000000000000000000000000000000000000000000000'))
```

----

### ST_AsTIFF


#### Signatures

```sql
BLOB ST_AsTIFF (rast RASTER)
BLOB ST_AsTIFF (rast RASTER, compression VARCHAR)
BLOB ST_AsTIFF (rast RASTER, options VARCHAR[])
```

#### Description

Returns the raster as the bytes of a GeoTIFF file.

`compression` is a GeoTIFF compression name: `NONE`, `LZW`, `DEFLATE` or `PACKBITS` (JPEG and ZSTD are not available in the bundled GDAL); `options` is a list of `NAME=VALUE` GeoTIFF creation options. The file carries the coordinate system as regular GeoTIFF keys.

`RASTER` values are stored uncompressed, because compressing makes writing a raster 10 to 50 times slower. The result of this function is itself a valid `RASTER`: `ST_AsTIFF(rast, 'DEFLATE')::RASTER` is a compressed raster that every function reads transparently, at the price of slower reads.

#### Example

```sql
SELECT octet_length(ST_AsTIFF(ST_AddBand(ST_MakeEmptyRaster(3, 2, 0, 0, 1), '8BUI'), 'LZW')) > 0;
----
true
```

----

### ST_AsTWKB


#### Signature

```sql
BLOB ST_AsTWKB (geom GEOMETRY, precision INTEGER)
```

#### Description

Encodes geometry as Tiny WKB (TWKB) with specified coordinate precision

#### Example

```sql
SELECT ST_AsTWKB(ST_Point(1, 2), 0)
```

----

### ST_AsWKB


#### Signatures

```sql
BLOB ST_AsWKB (geom GEOMETRY)
BLOB ST_AsWKB (geog GEOG)
```

#### Description

Returns the Well-Known Binary (WKB) representation of the geometry

#### Example

```sql
st_aswkb(ST_GeomFromWKB(X'01010000000000000000000000000000000000000000000000000'))
```

----

### ST_Azimuth


#### Signatures

```sql
DOUBLE ST_Azimuth (origin GEOMETRY, target GEOMETRY)
DOUBLE ST_Azimuth (origin POINT_2D, target POINT_2D)
DOUBLE ST_Azimuth (origin GEOG, target GEOG)
```

#### Description

Returns the azimuth (a clockwise angle measured from north) of two points in radian.

#### Example

```sql
SELECT degrees(ST_Azimuth(ST_Point(0, 0), ST_Point(0, 1)));
----
90.0
```

----

### ST_Band


#### Signatures

```sql
RASTER ST_Band (rast RASTER)
RASTER ST_Band (rast RASTER, nband INTEGER)
RASTER ST_Band (rast RASTER, nbands INTEGER[])
```

#### Description

Returns a raster made of one or more bands of the input raster.

`nband` is a 1-based band number (default 1); `nbands` is a list of band numbers, which may repeat or reorder bands.

#### Example

```sql
SELECT ST_NumBands(ST_Band(rast, [3, 1])) FROM (SELECT ST_AddBand(ST_AddBand(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '8BUI'), '8BUI'), '8BUI') AS rast);
----
2
```

----

### ST_BandIsNoData


#### Signatures

```sql
BOOLEAN ST_BandIsNoData (rast RASTER)
BOOLEAN ST_BandIsNoData (rast RASTER, band INTEGER)
BOOLEAN ST_BandIsNoData (rast RASTER, band INTEGER, forcechecking BOOLEAN)
BOOLEAN ST_BandIsNoData (rast RASTER, forcechecking BOOLEAN)
```

#### Description

Returns true if every pixel of the band (1-based, default 1) is NODATA. The pixels are always inspected, so `forcechecking` is accepted for compatibility and ignored.

#### Example

```sql
SELECT ST_BandIsNoData(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '8BUI', 7, 7));
----
true
```

----

### ST_BandMetaData


#### Signatures

```sql
STRUCT(pixeltype VARCHAR, nodatavalue DOUBLE, isoutdb BOOLEAN, path VARCHAR) ST_BandMetaData (rast RASTER)
STRUCT(pixeltype VARCHAR, nodatavalue DOUBLE, isoutdb BOOLEAN, path VARCHAR) ST_BandMetaData (rast RASTER, band INTEGER)
```

#### Description

Returns the pixel type and NODATA value (NULL if none) of a band (1-based, default 1) as a struct. `isoutdb` is always false and `path` always NULL: bands are always stored in the raster value.

#### Example

```sql
SELECT ST_BandMetaData(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '16BSI', 0, -1));
----
{'pixeltype': 16BSI, 'nodatavalue': -1.0, 'isoutdb': false, 'path': NULL}
```

----

### ST_BandNoDataValue


#### Signatures

```sql
DOUBLE ST_BandNoDataValue (rast RASTER)
DOUBLE ST_BandNoDataValue (rast RASTER, band INTEGER)
```

#### Description

Returns the NODATA value of a band (1-based, default 1), or NULL if the band has none.

#### Example

```sql
SELECT ST_BandNoDataValue(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '16BSI', 0, -1));
----
-1.0
```

----

### ST_BandPixelType


#### Signatures

```sql
VARCHAR ST_BandPixelType (rast RASTER)
VARCHAR ST_BandPixelType (rast RASTER, band INTEGER)
```

#### Description

Returns the pixel type of a band (1-based, default 1): `8BUI`, `8BSI`, `16BUI`, `16BSI`, `32BUI`, `32BSI`, `32BF` or `64BF`.

#### Example

```sql
SELECT ST_BandPixelType(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '16BSI'));
----
16BSI
```

----

### ST_Boundary


#### Signature

```sql
GEOMETRY ST_Boundary (geom GEOMETRY)
```

#### Description

Returns the "boundary" of a geometry

----

### ST_BoundingDiagonal


#### Signature

```sql
GEOMETRY ST_BoundingDiagonal (geom GEOMETRY)
```

#### Description

Returns the diagonal of the bounding box as a linestring

----

### ST_Box2dFromGeoHash


#### Signature

```sql
GEOMETRY ST_Box2dFromGeoHash (hash VARCHAR)
```

#### Description

Returns the bounding box polygon of a GeoHash cell

#### Example

```sql
SELECT ST_AsText(ST_Box2dFromGeoHash('dr5regw3p'))
```

----

### ST_Buffer


#### Signatures

```sql
GEOMETRY ST_Buffer (geom GEOMETRY, distance DOUBLE)
GEOMETRY ST_Buffer (geom GEOMETRY, distance DOUBLE, num_triangles INTEGER)
GEOMETRY ST_Buffer (geom GEOMETRY, distance DOUBLE, num_triangles INTEGER, cap_style VARCHAR, join_style VARCHAR, mitre_limit DOUBLE)
GEOG ST_Buffer (geog GEOG, distance DOUBLE)
GEOG ST_Buffer (geog GEOG, distance DOUBLE, num_triangles INTEGER)
```

#### Description

Returns a buffer around the input geometry at the target distance

`geom` is the input geometry.

`distance` is the target distance for the buffer, using the same units as the input geometry.

`num_triangles` represents how many triangles that will be produced to approximate a quarter circle. The larger the number, the smoother the resulting geometry. The default value is 8.

`cap_style` must be one of "CAP_ROUND", "CAP_FLAT", "CAP_SQUARE". This parameter is case-insensitive.

`join_style` must be one of "JOIN_ROUND", "JOIN_MITRE", "JOIN_BEVEL". This parameter is case-insensitive.

`mitre_limit` only applies when `join_style` is "JOIN_MITRE". It is the ratio of the distance from the corner to the mitre point to the corner radius. The default value is 1.0.

This is a planar operation and will not take into account the curvature of the earth.

----

### ST_BuildArea


#### Signature

```sql
GEOMETRY ST_BuildArea (geom GEOMETRY)
```

#### Description

Creates a polygonal geometry by attempting to "fill in" the input geometry.

Unlike ST_Polygonize, this function does not fill in holes.

----

### ST_Centroid


#### Signatures

```sql
GEOMETRY ST_Centroid (geom GEOMETRY)
POINT_2D ST_Centroid (point POINT_2D)
POINT_2D ST_Centroid (linestring LINESTRING_2D)
POINT_2D ST_Centroid (polygon POLYGON_2D)
POINT_2D ST_Centroid (box BOX_2D)
POINT_2D ST_Centroid (box BOX_2DF)
```

#### Description

Returns the centroid of a geometry

----

### ST_ChaikinSmoothing


#### Signature

```sql
GEOMETRY ST_ChaikinSmoothing (geom GEOMETRY, iterations INTEGER)
```

#### Description

Smooths a geometry using Chaikin's corner-cutting algorithm

#### Example

```sql
SELECT ST_AsText(ST_ChaikinSmoothing(ST_GeomFromText('LINESTRING(0 0, 5 10, 10 0)'), 1))
```

----

### ST_Clip


#### Signatures

```sql
RASTER ST_Clip (rast RASTER, geom GEOMETRY)
RASTER ST_Clip (rast RASTER, geom GEOMETRY, nodataval DOUBLE)
RASTER ST_Clip (rast RASTER, geom GEOMETRY, nodataval DOUBLE, crop BOOLEAN)
RASTER ST_Clip (rast RASTER, geom GEOMETRY, nodataval DOUBLE, crop BOOLEAN, touched BOOLEAN)
RASTER ST_Clip (rast RASTER, geom GEOMETRY, crop BOOLEAN)
RASTER ST_Clip (rast RASTER, geom GEOMETRY, crop BOOLEAN, touched BOOLEAN)
RASTER ST_Clip (rast RASTER, band INTEGER, geom GEOMETRY)
RASTER ST_Clip (rast RASTER, band INTEGER, geom GEOMETRY, nodataval DOUBLE)
RASTER ST_Clip (rast RASTER, band INTEGER, geom GEOMETRY, nodataval DOUBLE, crop BOOLEAN)
RASTER ST_Clip (rast RASTER, band INTEGER, geom GEOMETRY, nodataval DOUBLE, crop BOOLEAN, touched BOOLEAN)
RASTER ST_Clip (rast RASTER, band INTEGER, geom GEOMETRY, crop BOOLEAN)
RASTER ST_Clip (rast RASTER, band INTEGER, geom GEOMETRY, crop BOOLEAN, touched BOOLEAN)
```

#### Description

Returns the raster clipped by a geometry: pixels outside of the geometry become NODATA.

With `band` (1-based) the result has only that band; otherwise all bands are clipped. `nodataval` is the NODATA value of the result; by default the band's own NODATA value is used, or the smallest value of the pixel type if it has none. With `crop` (the default) the result is cut to the pixels that intersect the bounding box of the geometry; otherwise it keeps the size of the input. A pixel is inside when its centre is inside the geometry, or, if `touched` is true, when the geometry touches it.

The geometry must be in the coordinate system of the raster. Returns NULL if the geometry is empty or does not overlap the raster (PostGIS returns an empty raster). The variants taking lists of bands and NODATA values are not available.

#### Example

```sql
SELECT ST_Width(r), ST_Height(r) FROM (SELECT ST_Clip(ST_AddBand(ST_MakeEmptyRaster(10, 10, 0, 10, 1), '8BUI', 1, 0), ST_MakeEnvelope(2, 2, 5, 6)) AS r);
----
3    4
```

----

### ST_ClipByBox2D


#### Signature

```sql
GEOMETRY ST_ClipByBox2D (geom GEOMETRY, box GEOMETRY)
```

#### Description

Clips a geometry by a bounding box

----

### ST_ClosestPoint


#### Signature

```sql
GEOMETRY ST_ClosestPoint (geom1 GEOMETRY, geom2 GEOMETRY)
```

#### Description

Returns the closest point on the first geometry to the second geometry

----

### ST_Collect


#### Signature

```sql
GEOMETRY ST_Collect (geoms GEOMETRY[])
```

#### Description

Collects a list of geometries into a collection geometry.
- If all geometries are `POINT`'s, a `MULTIPOINT` is returned.
- If all geometries are `LINESTRING`'s, a `MULTILINESTRING` is returned.
- If all geometries are `POLYGON`'s, a `MULTIPOLYGON` is returned.
- Otherwise if the input collection contains a mix of geometry types, a `GEOMETRYCOLLECTION` is returned.

Empty and `NULL` geometries are ignored. If all geometries are empty or `NULL`, a `GEOMETRYCOLLECTION EMPTY` is returned.

#### Example

```sql
-- With all POINT's, a MULTIPOINT is returned
SELECT ST_Collect([ST_Point(1, 2), ST_Point(3, 4)]);
----
MULTIPOINT (1 2, 3 4)

-- With mixed geometry types, a GEOMETRYCOLLECTION is returned
SELECT ST_Collect([ST_Point(1, 2), ST_GeomFromText('LINESTRING(3 4, 5 6)')]);
----
GEOMETRYCOLLECTION (POINT (1 2), LINESTRING (3 4, 5 6))

-- Note that the empty geometry is ignored, so the result is a MULTIPOINT
SELECT ST_Collect([ST_Point(1, 2), NULL, ST_GeomFromText('GEOMETRYCOLLECTION EMPTY')]);
----
MULTIPOINT (1 2)

-- If all geometries are empty or NULL, a GEOMETRYCOLLECTION EMPTY is returned
SELECT ST_Collect([NULL, ST_GeomFromText('GEOMETRYCOLLECTION EMPTY')]);
----
GEOMETRYCOLLECTION EMPTY

-- Tip: You can use the `ST_Collect` function together with the `list()` aggregate function to collect multiple rows of geometries into a single geometry collection:

CREATE TABLE points (geom GEOMETRY);

INSERT INTO points VALUES (ST_Point(1, 2)), (ST_Point(3, 4));

SELECT ST_Collect(list(geom)) FROM points;
----
MULTIPOINT (1 2, 3 4)
```

----

### ST_CollectionExtract


#### Signatures

```sql
GEOMETRY ST_CollectionExtract (geom GEOMETRY, type INTEGER)
GEOMETRY ST_CollectionExtract (geom GEOMETRY)
```

#### Description

Extracts geometries from a GeometryCollection into a typed multi geometry.

If the input geometry is a GeometryCollection, the function will return a multi geometry, determined by the `type` parameter.
- if `type` = 1, returns a MultiPoint containing all the Points in the collection
- if `type` = 2, returns a MultiLineString containing all the LineStrings in the collection
- if `type` = 3, returns a MultiPolygon containing all the Polygons in the collection

If no `type` parameters is provided, the function will return a multi geometry matching the highest "surface dimension"
of the contained geometries. E.g. if the collection contains only Points, a MultiPoint will be returned. But if the
collection contains both Points and LineStrings, a MultiLineString will be returned. Similarly, if the collection
contains Polygons, a MultiPolygon will be returned. Contained geometries of a lower surface dimension will be ignored.

If the input geometry contains nested GeometryCollections, their geometries will be extracted recursively and included
into the final multi geometry as well.

If the input geometry is not a GeometryCollection, the function will return the input geometry as is.

#### Example

```sql
SELECT ST_CollectionExtract('MULTIPOINT(1 2, 3 4)'::GEOMETRY, 1);
-- MULTIPOINT (1 2, 3 4)
```

----

### ST_ColorMap


#### Signatures

```sql
RASTER ST_ColorMap (rast RASTER)
RASTER ST_ColorMap (rast RASTER, nband INTEGER)
RASTER ST_ColorMap (rast RASTER, nband INTEGER, colormap VARCHAR)
RASTER ST_ColorMap (rast RASTER, nband INTEGER, colormap VARCHAR, method VARCHAR)
RASTER ST_ColorMap (rast RASTER, colormap VARCHAR)
RASTER ST_ColorMap (rast RASTER, colormap VARCHAR, method VARCHAR)
```

#### Description

Returns a raster of up to four `8BUI` bands (grey, RGB or RGBA) that renders band `nband` (1-based, default 1) with a colormap.

`colormap` is the name of a predefined colormap (`grayscale`, `pseudocolor`, `fire`, `bluered`) or a custom colormap: one entry per line, each a value followed by one to four colour components between 0 and 255 (separated by spaces, commas or colons). The value is a pixel value, a percentage of the range between the smallest and largest pixel value (`50%`), or `nv` for NODATA pixels. The result has as many bands as the longest entry.

`method` is `INTERPOLATE` (the default: colours are blended linearly between entries, and values beyond the first or last entry take its colour), `EXACT` (only pixels equal to an entry are coloured) or `NEAREST` (the colour of the closest entry). Pixels without a colour are 0 in all bands. Predefined colormaps are always interpolated.

#### Example

```sql
SELECT ST_NumBands(ST_ColorMap(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '8BUI', 5), 1, 'fire'));
----
4
```

----

### ST_ConcaveHull


#### Signature

```sql
GEOMETRY ST_ConcaveHull (geom GEOMETRY, ratio DOUBLE, allowHoles BOOLEAN)
```

#### Description

Returns the 'concave' hull of the input geometry, containing all of the source input's points, and which can be used to create polygons from points. The ratio parameter dictates the level of concavity; 1.0 returns the convex hull; and 0 indicates to return the most concave hull possible. Set allowHoles to a non-zero value to allow output containing holes.

----

### ST_Contains


#### Signatures

```sql
BOOLEAN ST_Contains (geom1 POLYGON_2D, geom2 POINT_2D)
BOOLEAN ST_Contains (geom1 GEOMETRY, geom2 GEOMETRY)
BOOLEAN ST_Contains (rast1 RASTER, rast2 RASTER)
BOOLEAN ST_Contains (rast1 RASTER, nband1 INTEGER, rast2 RASTER, nband2 INTEGER)
```

#### Description

Returns true if the first geometry contains the second geometry

In contrast to `ST_ContainsProperly`, this function will also return true if `geom2` is contained strictly on the boundary of `geom1`.
A geometry always `ST_Contains` itself, but does not `ST_ContainsProperly` itself.

----

### ST_ContainsProperly


#### Signature

```sql
BOOLEAN ST_ContainsProperly (geom1 GEOMETRY, geom2 GEOMETRY)
```

#### Description

Returns true if the first geometry \"properly\" contains the second geometry

In contrast to `ST_Contains`, this function does not return true if `geom2` is contained strictly on the boundary of `geom1`.
A geometry always `ST_Contains` itself, but does not `ST_ContainsProperly` itself.

----

### ST_Contour


#### Signatures

```sql
STRUCT(geom GEOMETRY, id INTEGER, "value" DOUBLE)[] ST_Contour (rast RASTER)
STRUCT(geom GEOMETRY, id INTEGER, "value" DOUBLE)[] ST_Contour (rast RASTER, bandnumber INTEGER)
STRUCT(geom GEOMETRY, id INTEGER, "value" DOUBLE)[] ST_Contour (rast RASTER, bandnumber INTEGER, level_interval DOUBLE)
STRUCT(geom GEOMETRY, id INTEGER, "value" DOUBLE)[] ST_Contour (rast RASTER, bandnumber INTEGER, level_interval DOUBLE, level_base DOUBLE)
STRUCT(geom GEOMETRY, id INTEGER, "value" DOUBLE)[] ST_Contour (rast RASTER, bandnumber INTEGER, level_interval DOUBLE, level_base DOUBLE, fixed_levels DOUBLE[])
STRUCT(geom GEOMETRY, id INTEGER, "value" DOUBLE)[] ST_Contour (rast RASTER, bandnumber INTEGER, level_interval DOUBLE, level_base DOUBLE, fixed_levels DOUBLE[], polygonize BOOLEAN)
```

#### Description

Returns the contour lines of a band as a list of structs `(geom, id, value)`. PostGIS returns a set of rows: use `UNNEST(..., recursive := true)` to get the same shape.

Contours are drawn every `level_interval` (default 100) starting from `level_base` (default 0), or at the `fixed_levels` if that list is not empty. `bandnumber` is 1-based and defaults to 1; NODATA pixels are ignored. With `polygonize` the result is polygons of the areas between consecutive levels instead of lines, and `value` is the lower bound that GDAL reports for each area. Pixel values are taken at pixel centres; in the outer half pixel of the raster the surface is the value of the outermost pixel centres.

#### Example

```sql
SELECT c.id, c.value, ST_GeometryType(c.geom) FROM (SELECT UNNEST(ST_Contour(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(3, 3, 0, 3, 1), '32BF', 0), 2, 2, 10), 1, 100.0, 0.0, [5.0])) AS c);
----
0    5.0    LINESTRING
```

----

### ST_ConvexHull


#### Signatures

```sql
GEOMETRY ST_ConvexHull (geom GEOMETRY)
GEOMETRY ST_ConvexHull (rast RASTER)
```

#### Description

Returns the convex hull enclosing the geometry

----

### ST_CoordDim


#### Signature

```sql
INTEGER ST_CoordDim (geom GEOMETRY)
```

#### Description

Returns the coordinate dimension of a geometry

----

### ST_Count


#### Signatures

```sql
BIGINT ST_Count (rast RASTER)
BIGINT ST_Count (rast RASTER, nband INTEGER)
BIGINT ST_Count (rast RASTER, nband INTEGER, exclude_nodata_value BOOLEAN)
BIGINT ST_Count (rast RASTER, exclude_nodata_value BOOLEAN)
```

#### Description

Returns the number of pixels of a band (1-based, default 1) that are not NODATA, or the number of all pixels if `exclude_nodata_value` is false.

#### Example

```sql
SELECT ST_Count(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '8BUI', 1, 0), 1, 1, NULL));
----
3
```

----

### ST_CoverageClean


#### Signatures

```sql
GEOMETRY ST_CoverageClean (geoms GEOMETRY[], snapping_distance DOUBLE, gap_maximum_width DOUBLE)
GEOMETRY ST_CoverageClean (geoms GEOMETRY[], snapping_distance DOUBLE)
GEOMETRY ST_CoverageClean (geoms GEOMETRY[])
```

#### Description

Aligns the edges of a list of polygons whose edges are meant to align but are in fact exact matches.

Returns a collection of fixed polygons with the same size and order as the input polygons. EMPTY will be used in place of collapsed polygons.

----

### ST_CoverageInvalidEdges


#### Signatures

```sql
GEOMETRY ST_CoverageInvalidEdges (geoms GEOMETRY[], tolerance DOUBLE)
GEOMETRY ST_CoverageInvalidEdges (geoms GEOMETRY[])
```

#### Description

Returns the invalid edges in a polygonal coverage, which are edges that are not shared by two polygons.
Returns NULL if the input is not a polygonal coverage, or if the input is valid.
Tolerance is 0 by default.

----

### ST_CoverageSimplify


#### Signatures

```sql
GEOMETRY ST_CoverageSimplify (geoms GEOMETRY[], tolerance DOUBLE, simplify_boundary BOOLEAN)
GEOMETRY ST_CoverageSimplify (geoms GEOMETRY[], tolerance DOUBLE)
```

#### Description

Simplify the edges in a polygonal coverage, preserving the coverange by ensuring that the there are no seams between the resulting simplified polygons.

By default, the boundary of the coverage is also simplified, but this can be controlled with the optional third 'simplify_boundary' parameter.

----

### ST_CoverageUnion


#### Signature

```sql
GEOMETRY ST_CoverageUnion (geoms GEOMETRY[])
```

#### Description

Union all geometries in a polygonal coverage into a single geometry.
This may be faster than using `ST_Union`, but may use more memory.

----

### ST_CoveredBy


#### Signature

```sql
BOOLEAN ST_CoveredBy (geom1 GEOMETRY, geom2 GEOMETRY)
```

#### Description

Returns true if geom1 is "covered by" geom2

----

### ST_Covers


#### Signature

```sql
BOOLEAN ST_Covers (geom1 GEOMETRY, geom2 GEOMETRY)
```

#### Description

Returns true if the geom1 "covers" geom2

----

### ST_Crosses


#### Signature

```sql
BOOLEAN ST_Crosses (geom1 GEOMETRY, geom2 GEOMETRY)
```

#### Description

Returns true if geom1 "crosses" geom2

----

### ST_DelaunayTriangles


#### Signature

```sql
GEOMETRY ST_DelaunayTriangles (geom GEOMETRY)
```

#### Description

Returns Delaunay triangulation of input geometry vertices

----

### ST_DFullyWithin


#### Signature

```sql
BOOLEAN ST_DFullyWithin (geom1 GEOMETRY, geom2 GEOMETRY, distance DOUBLE)
```

#### Description

Returns true if every point of geom1 is within the given distance of geom2. Currently restricted to POINT inputs (reduces to ST_DWithin for points).

#### Example

```sql
SELECT ST_DFullyWithin(ST_Point(0, 0), ST_Point(1, 0), 2.0)
```

----

### ST_Difference


#### Signature

```sql
GEOMETRY ST_Difference (geom1 GEOMETRY, geom2 GEOMETRY)
```

#### Description

Returns the "difference" between two geometries

----

### ST_Dimension


#### Signature

```sql
INTEGER ST_Dimension (geom GEOMETRY)
```

#### Description

Returns the "topological dimension" of a geometry.

- For POINT and MULTIPOINT geometries, returns `0`
- For LINESTRING and MULTILINESTRING, returns `1`
- For POLYGON and MULTIPOLYGON, returns `2`
- For GEOMETRYCOLLECTION, returns the maximum dimension of the contained geometries, or 0 if the collection is empty

#### Example

```sql
SELECT ST_Dimension('POLYGON((0 0, 0 1, 1 1, 1 0, 0 0))'::GEOMETRY);
----
2
```

----

### ST_Disjoint


#### Signature

```sql
BOOLEAN ST_Disjoint (geom1 GEOMETRY, geom2 GEOMETRY)
```

#### Description

Returns true if the geometries are disjoint

----

### ST_Distance


#### Signatures

```sql
DOUBLE ST_Distance (point1 POINT_2D, point2 POINT_2D)
DOUBLE ST_Distance (point POINT_2D, linestring LINESTRING_2D)
DOUBLE ST_Distance (linestring LINESTRING_2D, point POINT_2D)
DOUBLE ST_Distance (geom1 GEOMETRY, geom2 GEOMETRY)
DOUBLE ST_Distance (geog1 GEOG, geog2 GEOG)
```

#### Description

Returns the planar distance between two geometries

#### Example

```sql
SELECT ST_Distance('POINT (0 0)'::GEOMETRY, 'POINT (3 4)'::GEOMETRY);
----
5.0

-- Z coordinates are ignored
SELECT ST_Distance('POINT Z (0 0 0)'::GEOMETRY, 'POINT Z (3 4 5)'::GEOMETRY);
----
5.0
```

----

### ST_Distance_GEOS


#### Signature

```sql
DOUBLE ST_Distance_GEOS (geom1 GEOMETRY, geom2 GEOMETRY)
```

#### Description

Returns the planar distance between two geometries

----

### ST_Distance_Sphere


#### Signatures

```sql
DOUBLE ST_Distance_Sphere (geom1 GEOMETRY, geom2 GEOMETRY)
DOUBLE ST_Distance_Sphere (point1 POINT_2D, point2 POINT_2D)
```

#### Description

Returns the haversine (great circle) distance between two geometries.

- Only supports POINT geometries.
- Returns the distance in meters.
- The input is expected to be in WGS84 (EPSG:4326) coordinates, using a [latitude, longitude] axis order.

----

### ST_Distance_Spheroid


#### Signature

```sql
DOUBLE ST_Distance_Spheroid (p1 POINT_2D, p2 POINT_2D)
```

#### Description

Returns the distance between two geometries in meters using an ellipsoidal model of the earths surface

The input geometry is assumed to be in the [EPSG:4326](https://en.wikipedia.org/wiki/World_Geodetic_System) coordinate system (WGS84), with [latitude, longitude] axis order and the distance limit is expected to be in meters. This function uses the [GeographicLib](https://geographiclib.sourceforge.io/) library to solve the [inverse geodesic problem](https://en.wikipedia.org/wiki/Geodesics_on_an_ellipsoid#Solution_of_the_direct_and_inverse_problems), calculating the distance between two points using an ellipsoidal model of the earth. This is a highly accurate method for calculating the distance between two arbitrary points taking the curvature of the earths surface into account, but is also the slowest.

#### Example

```sql
-- Note: the coordinates are in WGS84 and [latitude, longitude] axis order
-- Whats the distance between New York and Amsterdam (JFK and AMS airport)?
SELECT st_distance_spheroid(
st_point(40.6446, -73.7797),
st_point(52.3130, 4.7725)
);
----
5863418.7459356235
-- Roughly 5863km!
```

----

### ST_Dump


#### Signature

```sql
STRUCT(geom GEOMETRY, path INTEGER[])[] ST_Dump (geom GEOMETRY)
```

#### Description

Dumps a geometry into a list of sub-geometries and their "path" in the original geometry.

You can use the `unnest(res, recursive := true)` function to explode the resulting list of structs into multiple rows.

#### Example

```sql
SELECT ST_Dump('MULTIPOINT(1 2, 3 4)'::GEOMETRY);
----
[{'geom': 'POINT(1 2)', 'path': [0]}, {'geom': 'POINT(3 4)', 'path': [1]}]

SELECT unnest(ST_Dump('MULTIPOINT(1 2, 3 4)'::GEOMETRY), recursive := true);
-- ┌─────────────┬─────────┐
-- │    geom     │  path   │
-- │  geometry   │ int32[] │
-- ├─────────────┼─────────┤
-- │ POINT (1 2) │ [1]     │
-- │ POINT (3 4) │ [2]     │
-- └─────────────┴─────────┘
```

----

### ST_DumpAsPolygons


#### Signatures

```sql
STRUCT(geom GEOMETRY, val DOUBLE)[] ST_DumpAsPolygons (rast RASTER)
STRUCT(geom GEOMETRY, val DOUBLE)[] ST_DumpAsPolygons (rast RASTER, band INTEGER)
STRUCT(geom GEOMETRY, val DOUBLE)[] ST_DumpAsPolygons (rast RASTER, band INTEGER, exclude_nodata_value BOOLEAN)
```

#### Description

Vectorizes a band: returns a list of structs `(geom, val)` with one polygon per connected area of pixels that share a value. PostGIS returns a set of rows: use `UNNEST(..., recursive := true)` to get the same shape.

`band` is 1-based and defaults to 1. NODATA pixels are left out unless `exclude_nodata_value` is false. Pixels that only touch at a corner are not connected. Bands of type `32BUI`, `32BF` and `64BF` are compared as 32-bit floats.

#### Example

```sql
SELECT UNNEST(ST_DumpAsPolygons(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(2, 1, 0, 1, 1), '8BUI', 1), 2, 1, 5)), recursive := true);
----
POLYGON ((0 1, 0 0, 1 0, 1 1, 0 1))    1.0
POLYGON ((1 1, 1 0, 2 0, 2 1, 1 1))    5.0
```

----

### ST_DumpValues


#### Signatures

```sql
DOUBLE[][] ST_DumpValues (rast RASTER)
DOUBLE[][] ST_DumpValues (rast RASTER, nband INTEGER)
DOUBLE[][] ST_DumpValues (rast RASTER, nband INTEGER, exclude_nodata_value BOOLEAN)
```

#### Description

Returns the pixel values of a band as a list of rows, each a list of values: `result[y][x]` is the pixel at column `x`, row `y` (both 1-based).

`nband` is 1-based and defaults to 1. NODATA pixels are NULL unless `exclude_nodata_value` is false. The variant of PostGIS that returns one row per band is not available: call the function once per band.

#### Example

```sql
SELECT ST_DumpValues(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '8BUI', 1, 0), 2, 1, NULL));
----
[[1.0, NULL], [1.0, 1.0]]
```

----

### ST_DWithin


#### Signatures

```sql
BOOLEAN ST_DWithin (geom1 GEOMETRY, geom2 GEOMETRY, distance DOUBLE)
BOOLEAN ST_DWithin (geog1 GEOG, geog2 GEOG, distance DOUBLE)
```

#### Description

Returns if two geometries are within a target distance of each-other

----

### ST_DWithin_GEOS


#### Signature

```sql
BOOLEAN ST_DWithin_GEOS (geom1 GEOMETRY, geom2 GEOMETRY, distance DOUBLE)
```

#### Description

Returns true if two geometries are within a target distance of each-other

----

### ST_DWithin_Spheroid


#### Signature

```sql
BOOLEAN ST_DWithin_Spheroid (p1 POINT_2D, p2 POINT_2D, distance DOUBLE)
```

#### Description

Returns if two POINT_2D's are within a target distance in meters, using an ellipsoidal model of the earths surface

The input geometry is assumed to be in the [EPSG:4326](https://en.wikipedia.org/wiki/World_Geodetic_System) coordinate system (WGS84), with [latitude, longitude] axis order and the distance is returned in meters. This function uses the [GeographicLib](https://geographiclib.sourceforge.io/) library to solve the [inverse geodesic problem](https://en.wikipedia.org/wiki/Geodesics_on_an_ellipsoid#Solution_of_the_direct_and_inverse_problems), calculating the distance between two points using an ellipsoidal model of the earth. This is a highly accurate method for calculating the distance between two arbitrary points taking the curvature of the earths surface into account, but is also the slowest.

----

### ST_EndPoint


#### Signatures

```sql
GEOMETRY ST_EndPoint (geom GEOMETRY)
POINT_2D ST_EndPoint (line LINESTRING_2D)
```

#### Description

Returns the end point of a LINESTRING.

----

### ST_Envelope


#### Signatures

```sql
GEOMETRY ST_Envelope (geom GEOMETRY)
GEOMETRY ST_Envelope (rast RASTER)
```

#### Description

Returns the minimum bounding rectangle of a geometry as a polygon geometry

----

### ST_Equals


#### Signature

```sql
BOOLEAN ST_Equals (geom1 GEOMETRY, geom2 GEOMETRY)
```

#### Description

Returns true if the geometries are "equal"

----

### ST_Expand


#### Signature

```sql
GEOMETRY ST_Expand (geom GEOMETRY, distance DOUBLE)
```

#### Description

Expand the input geometry by the specified distance, returning a polygon.

`geom` is the input geometry.

`distance` is the target distance for the expansion, using the same units as the input geometry.

This is a planar operation and will not take into account the curvature of the earth.

#### Example

```sql
SELECT ST_AsText(ST_Expand(ST_GeomFromText('POINT(20 30)'), 0.1));
```

----

### ST_Extent


#### Signature

```sql
BOX_2D ST_Extent (geom GEOMETRY)
```

#### Description

Returns the minimal bounding box enclosing the input geometry

----

### ST_Extent_Approx


#### Signature

```sql
BOX_2DF ST_Extent_Approx (geom GEOMETRY)
```

#### Description

Returns the approximate bounding box of a geometry, if available.

This function is only really used internally, and returns the cached bounding box of the geometry if it exists.
This function may be removed or renamed in the future.

----

### ST_ExteriorRing


#### Signatures

```sql
GEOMETRY ST_ExteriorRing (geom GEOMETRY)
LINESTRING_2D ST_ExteriorRing (polygon POLYGON_2D)
```

#### Description

Returns the exterior ring (shell) of a polygon geometry.

----

### ST_FlipCoordinates


#### Signatures

```sql
GEOMETRY ST_FlipCoordinates (geom GEOMETRY)
POINT_2D ST_FlipCoordinates (point POINT_2D)
LINESTRING_2D ST_FlipCoordinates (linestring LINESTRING_2D)
POLYGON_2D ST_FlipCoordinates (polygon POLYGON_2D)
BOX_2D ST_FlipCoordinates (box BOX_2D)
```

#### Description

Returns a new geometry with the coordinates of the input geometry "flipped" so that x = y and y = x

----

### ST_Force2D


#### Signature

```sql
GEOMETRY ST_Force2D (geom GEOMETRY)
```

#### Description

Forces the vertices of a geometry to have X and Y components

This function will drop any Z and M values from the input geometry, if present. If the input geometry is already 2D, it will be returned as is.

----

### ST_Force3DM


#### Signature

```sql
GEOMETRY ST_Force3DM (geom GEOMETRY, m DOUBLE)
```

#### Description

Forces the vertices of a geometry to have X, Y and M components

The following cases apply:
- If the input geometry has a Z component but no M component, the Z component will be replaced with the new M value.
- If the input geometry has a M component but no Z component, it will be returned as is.
- If the input geometry has both a Z component and a M component, the Z component will be removed.
- Otherwise, if the input geometry has neither a Z or M component, the new M value will be added to the vertices of the input geometry.

----

### ST_Force3DZ


#### Signature

```sql
GEOMETRY ST_Force3DZ (geom GEOMETRY, z DOUBLE)
```

#### Description

Forces the vertices of a geometry to have X, Y and Z components

The following cases apply:
- If the input geometry has a M component but no Z component, the M component will be replaced with the new Z value.
- If the input geometry has a Z component but no M component, it will be returned as is.
- If the input geometry has both a Z component and a M component, the M component will be removed.
- Otherwise, if the input geometry has neither a Z or M component, the new Z value will be added to the vertices of the input geometry.

----

### ST_Force4D


#### Signature

```sql
GEOMETRY ST_Force4D (geom GEOMETRY, z DOUBLE, m DOUBLE)
```

#### Description

Forces the vertices of a geometry to have X, Y, Z and M components

The following cases apply:
- If the input geometry has a Z component but no M component, the new M value will be added to the vertices of the input geometry.
- If the input geometry has a M component but no Z component, the new Z value will be added to the vertices of the input geometry.
- If the input geometry has both a Z component and a M component, the geometry will be returned as is.
- Otherwise, if the input geometry has neither a Z or M component, the new Z and M values will be added to the vertices of the input geometry.

----

### ST_ForceCollection


#### Signature

```sql
GEOMETRY ST_ForceCollection (geom GEOMETRY)
```

#### Description

Wraps a geometry in a GeometryCollection (no-op if already a collection)

#### Example

```sql
SELECT ST_AsText(ST_ForceCollection(ST_Point(1, 2)))
```

----

### ST_ForcePolygonCCW


#### Signature

```sql
GEOMETRY ST_ForcePolygonCCW (geom GEOMETRY)
```

#### Description

Forces polygon exterior rings to be counter-clockwise

----

### ST_ForcePolygonCW


#### Signature

```sql
GEOMETRY ST_ForcePolygonCW (geom GEOMETRY)
```

#### Description

Forces polygon exterior rings to be clockwise

----

### ST_FrechetDistance


#### Signature

```sql
DOUBLE ST_FrechetDistance (geom1 GEOMETRY, geom2 GEOMETRY)
```

#### Description

Returns the Frechet distance between two geometries

----

### ST_FromGDALRaster


#### Signatures

```sql
RASTER ST_FromGDALRaster (gdaldata BLOB)
RASTER ST_FromGDALRaster (gdaldata BLOB, srid INTEGER)
```

#### Description

Creates a raster from the bytes of a raster file in any format the bundled GDAL reads from memory (GeoTIFF and Erdas Imagine; see `ST_GDALDrivers()`).

`srid` overrides the coordinate system of the file with an EPSG code, without reprojecting. VRT files are rejected because they refer to other files: read them with `ST_ReadRaster`. All bands of the result share the pixel type of the first band.

#### Example

```sql
SELECT ST_Width(ST_FromGDALRaster(ST_AsGDALRaster(ST_AddBand(ST_MakeEmptyRaster(3, 2, 0, 0, 1), '8BUI'), 'GTiff')));
----
3
```

----

### ST_GeogFromText


#### Signature

```sql
GEOG ST_GeogFromText (wkt VARCHAR)
```

#### Description

Creates a GEOG from its WKT representation.

The coordinates are longitude and latitude in degrees on WGS84, in that order. Longitudes outside of [-180, 180] and latitudes outside of [-90, 90] raise an error. `ST_GeogFromText`, `ST_GeogFromWKT` and `ST_GeographyFromText` are the same function.

#### Example

```sql
SELECT ST_GeogFromText('POINT(4.3517 50.8503)');
```

----

### ST_GeogFromWKB


#### Signature

```sql
GEOG ST_GeogFromWKB (wkb BLOB)
```

#### Description

Creates a GEOG from its WKB representation.

The coordinates are longitude and latitude in degrees on WGS84, in that order. Longitudes outside of [-180, 180] and latitudes outside of [-90, 90] raise an error.

#### Example

```sql
SELECT ST_GeogFromWKB(ST_AsWKB(ST_Point(4.3517, 50.8503)));
```

----

### ST_GeogFromWKT


#### Signature

```sql
GEOG ST_GeogFromWKT (wkt VARCHAR)
```

#### Description

Creates a GEOG from its WKT representation.

The coordinates are longitude and latitude in degrees on WGS84, in that order. Longitudes outside of [-180, 180] and latitudes outside of [-90, 90] raise an error. `ST_GeogFromText`, `ST_GeogFromWKT` and `ST_GeographyFromText` are the same function.

#### Example

```sql
SELECT ST_GeogFromText('POINT(4.3517 50.8503)');
```

----

### ST_GeogPoint


#### Signature

```sql
GEOG ST_GeogPoint (longitude DOUBLE, latitude DOUBLE)
```

#### Description

Creates a GEOG point from a longitude and a latitude in degrees on WGS84

#### Example

```sql
SELECT ST_GeogPoint(4.3517, 50.8503);
```

----

### ST_GeographyFromText


#### Signature

```sql
GEOG ST_GeographyFromText (wkt VARCHAR)
```

#### Description

Creates a GEOG from its WKT representation.

The coordinates are longitude and latitude in degrees on WGS84, in that order. Longitudes outside of [-180, 180] and latitudes outside of [-90, 90] raise an error. `ST_GeogFromText`, `ST_GeogFromWKT` and `ST_GeographyFromText` are the same function.

#### Example

```sql
SELECT ST_GeogFromText('POINT(4.3517 50.8503)');
```

----

### ST_GeoHash


#### Signature

```sql
VARCHAR ST_GeoHash (geom GEOMETRY, precision INTEGER)
```

#### Description

Returns the GeoHash string of a geometry's centroid at the given precision

#### Example

```sql
SELECT ST_GeoHash(ST_Point(-74.006, 40.7128), 9);
```

----

### ST_GeometricMedian


#### Signature

```sql
GEOMETRY ST_GeometricMedian (geom GEOMETRY)
```

#### Description

Returns the geometric median of a geometry's vertices (Weiszfeld algorithm)

#### Example

```sql
SELECT ST_AsText(ST_GeometricMedian(ST_GeomFromText('MULTIPOINT(0 0, 10 0, 0 10)')))
```

----

### ST_GeometryN


#### Signature

```sql
GEOMETRY ST_GeometryN (geom GEOMETRY, n INTEGER)
```

#### Description

Returns the n-th geometry of a collection or multi-geometry, counting from 1 as in PostGIS and like `ST_PointN` and `ST_InteriorRingN`.

A geometry that is not a collection is its own first element. Returns NULL when `n` is out of range.

#### Example

```sql
SELECT ST_GeometryN('MULTIPOINT (0 0, 1 1, 2 2)'::GEOMETRY, 2);
```

----

### ST_GeometryType


#### Signatures

```sql
ANY ST_GeometryType (geom GEOMETRY)
ANY ST_GeometryType (point POINT_2D)
ANY ST_GeometryType (linestring LINESTRING_2D)
ANY ST_GeometryType (polygon POLYGON_2D)
```

#### Description

Returns a 'GEOMETRY_TYPE' enum identifying the input geometry type. Possible enum return types are: `POINT`, `LINESTRING`, `POLYGON`, `MULTIPOINT`, `MULTILINESTRING`, `MULTIPOLYGON`, and `GEOMETRYCOLLECTION`.

#### Example

```sql
SELECT DISTINCT ST_GeometryType(ST_GeomFromText('POINT(1 1)'));
----
POINT
```

----

### ST_GeomFromEWKB


#### Signature

```sql
GEOMETRY ST_GeomFromEWKB (ewkb BLOB)
```

#### Description

Creates a geometry from EWKB (Extended Well-Known Binary) data

#### Example

```sql
SELECT ST_AsText(ST_GeomFromEWKB(ST_AsEWKB(ST_Point(1, 2))))
```

----

### ST_GeomFromEWKT


#### Signature

```sql
GEOMETRY ST_GeomFromEWKT (ewkt VARCHAR)
```

#### Description

Parses an Extended WKT (EWKT) string, optionally with SRID prefix

#### Example

```sql
SELECT ST_AsText(ST_GeomFromEWKT('SRID=4326;POINT(1 2)'))
```

----

### ST_GeomFromGeoHash


#### Signature

```sql
GEOMETRY ST_GeomFromGeoHash (hash VARCHAR)
```

#### Description

Returns the center point of a GeoHash cell

#### Example

```sql
SELECT ST_AsText(ST_GeomFromGeoHash('dr5regw3p'))
```

----

### ST_GeomFromGeoJSON


#### Signatures

```sql
GEOMETRY ST_GeomFromGeoJSON (geojson JSON)
GEOMETRY ST_GeomFromGeoJSON (geojson VARCHAR)
```

#### Description

Deserializes a GEOMETRY from a GeoJSON fragment.

#### Example

```sql
SELECT ST_GeomFromGeoJSON('{"type": "Point", "coordinates": [1.0, 2.0]}');
----
POINT (1 2)
```

----

### ST_GeomFromGML


#### Signature

```sql
GEOMETRY ST_GeomFromGML (gml VARCHAR)
```

#### Description

Creates a geometry from a GML (Geography Markup Language) geometry element.

Accepts GML 2 and GML 3 geometry elements, with or without the `gml:` namespace prefix. The `srsName` attribute is ignored.

#### Example

```sql
SELECT ST_GeomFromGML('<gml:LineString><gml:coordinates>0,0 1,1</gml:coordinates></gml:LineString>');
----
LINESTRING (0 0, 1 1)
```

----

### ST_GeomFromHEXEWKB


#### Signature

```sql
GEOMETRY ST_GeomFromHEXEWKB (hexwkb VARCHAR)
```

#### Description

Deserialize a GEOMETRY from a HEX(E)WKB encoded string

DuckDB spatial doesn't currently differentiate between `WKB` and `EWKB`, so `ST_GeomFromHEXWKB` and `ST_GeomFromHEXEWKB` are just aliases of each other.

----

### ST_GeomFromHEXWKB


#### Signature

```sql
GEOMETRY ST_GeomFromHEXWKB (hexwkb VARCHAR)
```

#### Description

Deserialize a GEOMETRY from a HEX(E)WKB encoded string

DuckDB spatial doesn't currently differentiate between `WKB` and `EWKB`, so `ST_GeomFromHEXWKB` and `ST_GeomFromHEXEWKB` are just aliases of each other.

----

### ST_GeomFromKML


#### Signature

```sql
GEOMETRY ST_GeomFromKML (kml VARCHAR)
```

#### Description

Creates a geometry from a KML (Keyhole Markup Language) geometry element.

Accepts a single `Point`, `LineString`, `LinearRing`, `Polygon` or `MultiGeometry` element, not a whole KML document (use `ST_Read` to read KML files). A `MultiGeometry` is returned as a geometry collection.

#### Example

```sql
SELECT ST_GeomFromKML('<Point><coordinates>4.35,50.85</coordinates></Point>');
----
POINT (4.35 50.85)
```

----

### ST_GeomFromText


#### Signatures

```sql
GEOMETRY ST_GeomFromText (wkt VARCHAR)
GEOMETRY ST_GeomFromText (wkt VARCHAR, ignore_invalid BOOLEAN)
```

#### Description

Deserialize a GEOMETRY from a WKT encoded string

----

### ST_GeomFromTWKB


#### Signature

```sql
GEOMETRY ST_GeomFromTWKB (twkb BLOB)
```

#### Description

Decodes a Tiny WKB (TWKB) binary into a geometry

#### Example

```sql
SELECT ST_AsText(ST_GeomFromTWKB(ST_AsTWKB(ST_Point(1, 2), 0)))
```

----

### ST_GeomFromWKB


#### Signature

```sql
GEOMETRY ST_GeomFromWKB (wkb BLOB)
```

#### Description

Creates a geometry from Well-Known Binary (WKB) representation

#### Example

```sql
ST_GeomFromWKB(X'01010000000000000000000000000000000000000000000000')
```

----

### ST_GeoReference


#### Signatures

```sql
VARCHAR ST_GeoReference (rast RASTER)
VARCHAR ST_GeoReference (rast RASTER, format VARCHAR)
```

#### Description

Returns the georeference as the six lines of a world file: `scalex`, `skewy`, `skewx`, `scaley`, `upperleftx`, `upperlefty`, each with 10 decimals and followed by a newline.

`format` is `GDAL` (the default, upper-left corner of the upper-left pixel) or `ESRI` (centre of the upper-left pixel).

#### Example

```sql
SELECT string_split(rtrim(ST_GeoReference(ST_MakeEmptyRaster(10, 5, 100, 200, 2, -2, 0, 0)), chr(10)), chr(10));
----
[2.0000000000, 0.0000000000, 0.0000000000, -2.0000000000, 100.0000000000, 200.0000000000]
```

----

### ST_Grayscale


#### Signatures

```sql
RASTER ST_Grayscale (rast RASTER)
RASTER ST_Grayscale (rast RASTER, redband INTEGER)
RASTER ST_Grayscale (rast RASTER, redband INTEGER, greenband INTEGER)
RASTER ST_Grayscale (rast RASTER, redband INTEGER, greenband INTEGER, blueband INTEGER)
```

#### Description

Returns a single-band `8BUI` raster with the luminance of three bands holding red, green and blue values between 0 and 255: `0.2989 * R + 0.5870 * G + 0.1140 * B`.

The band numbers are 1-based and default to 1, 2 and 3. The `rastbandarg[]` variant of PostGIS and the `extenttype` argument are not available: the three bands come from one raster.

#### Example

```sql
SELECT ST_Value(ST_Grayscale(ST_AddBand(ST_AddBand(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '8BUI', 255), '8BUI', 0), '8BUI', 0)), 1, 1);
----
76.0
```

----

### ST_HasM


#### Signature

```sql
BOOLEAN ST_HasM (geom GEOMETRY)
```

#### Description

Check if the input geometry has M values.

#### Example

```sql
-- HasM for a 2D geometry
SELECT ST_HasM(ST_GeomFromText('POINT(1 1)'));
----
false

-- HasM for a 3DZ geometry
SELECT ST_HasM(ST_GeomFromText('POINT Z(1 1 1)'));
----
false

-- HasM for a 3DM geometry
SELECT ST_HasM(ST_GeomFromText('POINT M(1 1 1)'));
----
true

-- HasM for a 4D geometry
SELECT ST_HasM(ST_GeomFromText('POINT ZM(1 1 1 1)'));
----
true
```

----

### ST_HasNoBand


#### Signatures

```sql
BOOLEAN ST_HasNoBand (rast RASTER)
BOOLEAN ST_HasNoBand (rast RASTER, band INTEGER)
```

#### Description

Returns true if the raster has no band with the given number (1-based, default 1).

#### Example

```sql
SELECT ST_HasNoBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1));
----
true
```

----

### ST_HasZ


#### Signature

```sql
BOOLEAN ST_HasZ (geom GEOMETRY)
```

#### Description

Check if the input geometry has Z values.

#### Example

```sql
-- HasZ for a 2D geometry
SELECT ST_HasZ(ST_GeomFromText('POINT(1 1)'));
----
false

-- HasZ for a 3DZ geometry
SELECT ST_HasZ(ST_GeomFromText('POINT Z(1 1 1)'));
----
true

-- HasZ for a 3DM geometry
SELECT ST_HasZ(ST_GeomFromText('POINT M(1 1 1)'));
----
false

-- HasZ for a 4D geometry
SELECT ST_HasZ(ST_GeomFromText('POINT ZM(1 1 1 1)'));
----
true
```

----

### ST_HausdorffDistance


#### Signature

```sql
DOUBLE ST_HausdorffDistance (geom1 GEOMETRY, geom2 GEOMETRY)
```

#### Description

Returns the Hausdorff distance between two geometries

----

### ST_Height


#### Signature

```sql
INTEGER ST_Height (rast RASTER)
```

#### Description

Returns the height of the raster in pixels.

#### Example

```sql
SELECT ST_Height(ST_MakeEmptyRaster(10, 5, 0, 0, 1));
----
5
```

----

### ST_Hilbert


#### Signatures

```sql
UINTEGER ST_Hilbert (x DOUBLE, y DOUBLE, bounds BOX_2D)
UINTEGER ST_Hilbert (geom GEOMETRY, bounds BOX_2D)
UINTEGER ST_Hilbert (geom GEOMETRY)
UINTEGER ST_Hilbert (box BOX_2D, bounds BOX_2D)
UINTEGER ST_Hilbert (box BOX_2DF, bounds BOX_2DF)
```

#### Description

Encodes the X and Y values as the hilbert curve index for a curve covering the given bounding box.
If a geometry is provided, the center of the approximate bounding box is used as the point to encode.
If no bounding box is provided, the hilbert curve index is mapped to the full range of a single-precision float.
For the BOX_2D and BOX_2DF variants, the center of the box is used as the point to encode.

----

### ST_Hillshade


#### Signatures

```sql
RASTER ST_Hillshade (rast RASTER)
RASTER ST_Hillshade (rast RASTER, nband INTEGER)
RASTER ST_Hillshade (rast RASTER, nband INTEGER, pixeltype VARCHAR)
RASTER ST_Hillshade (rast RASTER, nband INTEGER, pixeltype VARCHAR, azimuth DOUBLE)
RASTER ST_Hillshade (rast RASTER, nband INTEGER, pixeltype VARCHAR, azimuth DOUBLE, altitude DOUBLE)
RASTER ST_Hillshade (rast RASTER, nband INTEGER, pixeltype VARCHAR, azimuth DOUBLE, altitude DOUBLE, max_bright DOUBLE)
RASTER ST_Hillshade (rast RASTER, nband INTEGER, pixeltype VARCHAR, azimuth DOUBLE, altitude DOUBLE, max_bright DOUBLE, scale DOUBLE)
RASTER ST_Hillshade (rast RASTER, nband INTEGER, pixeltype VARCHAR, azimuth DOUBLE, altitude DOUBLE, max_bright DOUBLE, scale DOUBLE, interpolate_nodata BOOLEAN)
```

#### Description

Returns the hypothetical illumination of an elevation band.

`azimuth` is the direction of the light source in degrees clockwise from north (default 315) and `altitude` its angle above the horizon in degrees (default 45). `max_bright` is the value of a fully lit pixel (default 255) and `scale` the ratio of vertical units to horizontal units (default 1). GDAL computes the illumination as an 8-bit value, so the result has 255 distinct levels between 0 and `max_bright`.

Computed by GDAL's DEM processing on the 3x3 neighbourhood of each pixel. Pixels on the border and next to NODATA pixels are computed from a neighbourhood that GDAL extrapolates, where PostGIS substitutes the value of the centre pixel, so those pixels can differ from PostGIS. `nband` is 1-based (default 1) and `pixeltype` the pixel type of the result (default `32BF`). NODATA pixels are NODATA in the result (-9999, or the largest value of the pixel type if it cannot hold -9999). `interpolate_nodata` must be false and the `customextent` variants are not available.

#### Example

```sql
SELECT round(ST_Value(ST_Hillshade(ST_AddBand(ST_MakeEmptyRaster(5, 5, 0, 5, 1), '32BF', 10)), 3, 3));
----
181.0
```

----

### ST_Histogram


#### Signatures

```sql
STRUCT(min DOUBLE, max DOUBLE, count BIGINT, "percent" DOUBLE)[] ST_Histogram (rast RASTER)
STRUCT(min DOUBLE, max DOUBLE, count BIGINT, "percent" DOUBLE)[] ST_Histogram (rast RASTER, nband INTEGER)
STRUCT(min DOUBLE, max DOUBLE, count BIGINT, "percent" DOUBLE)[] ST_Histogram (rast RASTER, nband INTEGER, exclude_nodata_value BOOLEAN)
STRUCT(min DOUBLE, max DOUBLE, count BIGINT, "percent" DOUBLE)[] ST_Histogram (rast RASTER, nband INTEGER, exclude_nodata_value BOOLEAN, bins INTEGER)
STRUCT(min DOUBLE, max DOUBLE, count BIGINT, "percent" DOUBLE)[] ST_Histogram (rast RASTER, nband INTEGER, exclude_nodata_value BOOLEAN, bins INTEGER, width DOUBLE[])
STRUCT(min DOUBLE, max DOUBLE, count BIGINT, "percent" DOUBLE)[] ST_Histogram (rast RASTER, nband INTEGER, exclude_nodata_value BOOLEAN, bins INTEGER, width DOUBLE[], right BOOLEAN)
STRUCT(min DOUBLE, max DOUBLE, count BIGINT, "percent" DOUBLE)[] ST_Histogram (rast RASTER, nband INTEGER, exclude_nodata_value BOOLEAN, bins INTEGER, right BOOLEAN)
STRUCT(min DOUBLE, max DOUBLE, count BIGINT, "percent" DOUBLE)[] ST_Histogram (rast RASTER, nband INTEGER, bins INTEGER)
STRUCT(min DOUBLE, max DOUBLE, count BIGINT, "percent" DOUBLE)[] ST_Histogram (rast RASTER, nband INTEGER, bins INTEGER, width DOUBLE[])
STRUCT(min DOUBLE, max DOUBLE, count BIGINT, "percent" DOUBLE)[] ST_Histogram (rast RASTER, nband INTEGER, bins INTEGER, width DOUBLE[], right BOOLEAN)
STRUCT(min DOUBLE, max DOUBLE, count BIGINT, "percent" DOUBLE)[] ST_Histogram (rast RASTER, nband INTEGER, bins INTEGER, right BOOLEAN)
```

#### Description

Returns the distribution of the pixel values of a band as a list of bins `(min, max, count, percent)`, where `percent` is the fraction of the counted pixels that fall in the bin. PostGIS returns a set of rows: use `UNNEST` to get the same shape.

`nband` is 1-based and defaults to 1; NODATA pixels are left out unless `exclude_nodata_value` is false. `bins` is the number of bins; 0 (the default) chooses it from the number of values (the square root for fewer than 30 values, Sturges' formula otherwise). `width` is a list of bin widths that is repeated to cover the value range, instead of equal-width bins. Bins include their lower bound and exclude their upper bound, except the last one; with `right` set to true the bins are listed from the largest value down, exclude their lower bound and include their upper bound.

#### Example

```sql
SELECT UNNEST(ST_Histogram(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '8BUI', 1), 1, 1, 5), 1, 2), recursive := true);
----
1.0    3.0    3    0.75
3.0    5.0    1    0.25
```

----

### ST_InteriorRingN


#### Signatures

```sql
GEOMETRY ST_InteriorRingN (geom GEOMETRY, n BIGINT)
LINESTRING_2D ST_InteriorRingN (polygon POLYGON_2D, n BIGINT)
```

#### Description

Returns the N-th interior ring (hole) of a POLYGON as a LINESTRING. Indexing is 1-based  (n = 1 returns the first interior ring). Returns NULL if the polygon is empty or has fewer than N interior rings.

#### Example

```sql
SELECT ST_AsText(ST_InteriorRingN(ST_GeomFromText('POLYGON((0 0,10 0,10 10,0 10,0 0),(2 2,4 2,4 4,2 4,2 2))'), 1));
```

----

### ST_InterpolatePoint


#### Signature

```sql
DOUBLE ST_InterpolatePoint (line GEOMETRY, point GEOMETRY)
```

#### Description

Computes the closest point on a LINESTRING to a given POINT and returns the interpolated M value of that point.

First argument must be a linestring and must have a M dimension. The second argument must be a point. 
Neither argument can be empty.

----

### ST_InterpolateRaster


#### Signatures

```sql
RASTER ST_InterpolateRaster (geom GEOMETRY, options VARCHAR, rast RASTER)
RASTER ST_InterpolateRaster (geom GEOMETRY, options VARCHAR, rast RASTER, bandnumber INTEGER)
```

#### Description

Interpolates a surface from the 3D points of a geometry onto the grid of a raster, and returns the raster with band `bandnumber` (1-based, default 1) replaced by the interpolated values.

Every vertex of the geometry, which must have Z values, is an input point. `options` names the algorithm and its parameters in the syntax of `gdal_grid`: `invdist[:power:2.0:smoothing:0.0:...]`, `invdistnn`, `average`, `nearest` and the other GDAL gridding algorithms (`linear` needs a GDAL built with QHull, which the bundled one is not). Values are computed at pixel centres. The raster must not be skewed; the geometry must be in its coordinate system.

#### Example

```sql
SELECT ST_DumpValues(ST_InterpolateRaster(ST_GeomFromText('MULTIPOINT Z ((0.5 0.5 10), (1.5 0.5 20))'), 'nearest', ST_AddBand(ST_MakeEmptyRaster(2, 1, 0, 1, 1), '32BF')));
----
[[10.0, 20.0]]
```

----

### ST_Intersection


#### Signatures

```sql
GEOMETRY ST_Intersection (geom1 GEOMETRY, geom2 GEOMETRY)
STRUCT(geom GEOMETRY, val DOUBLE)[] ST_Intersection (rast RASTER, geomin GEOMETRY)
STRUCT(geom GEOMETRY, val DOUBLE)[] ST_Intersection (rast RASTER, band INTEGER, geomin GEOMETRY)
STRUCT(geom GEOMETRY, val DOUBLE)[] ST_Intersection (geomin GEOMETRY, rast RASTER)
STRUCT(geom GEOMETRY, val DOUBLE)[] ST_Intersection (geomin GEOMETRY, rast RASTER, band INTEGER)
```

#### Description

Returns the intersection of two geometries

----

### ST_Intersects


#### Signatures

```sql
BOOLEAN ST_Intersects (box1 BOX_2D, box2 BOX_2D)
BOOLEAN ST_Intersects (geom1 GEOMETRY, geom2 GEOMETRY)
BOOLEAN ST_Intersects (geog1 GEOG, geog2 GEOG)
BOOLEAN ST_Intersects (rast RASTER, geom GEOMETRY)
BOOLEAN ST_Intersects (rast RASTER, geom GEOMETRY, nband INTEGER)
BOOLEAN ST_Intersects (geom GEOMETRY, rast RASTER)
BOOLEAN ST_Intersects (geom GEOMETRY, rast RASTER, nband INTEGER)
BOOLEAN ST_Intersects (rast RASTER, nband INTEGER, geom GEOMETRY)
BOOLEAN ST_Intersects (rast1 RASTER, rast2 RASTER)
BOOLEAN ST_Intersects (rast1 RASTER, nband1 INTEGER, rast2 RASTER, nband2 INTEGER)
```

#### Description

Returns true if two geometries intersect

----

### ST_Intersects_Extent


#### Signature

```sql
BOOLEAN ST_Intersects_Extent (geom1 GEOMETRY, geom2 GEOMETRY)
```

#### Description

Returns true if the extent of two geometries intersects

----

### ST_IsClosed


#### Signature

```sql
BOOLEAN ST_IsClosed (geom GEOMETRY)
```

#### Description

Check if a geometry is 'closed'

----

### ST_IsCollection


#### Signature

```sql
BOOLEAN ST_IsCollection (geom GEOMETRY)
```

#### Description

Returns true if geometry is a Multi* or GeometryCollection type

----

### ST_IsEmpty


#### Signatures

```sql
BOOLEAN ST_IsEmpty (geom GEOMETRY)
BOOLEAN ST_IsEmpty (linestring LINESTRING_2D)
BOOLEAN ST_IsEmpty (polygon POLYGON_2D)
```

#### Description

Returns true if the geometry is "empty".

----

### ST_IsPolygonCCW


#### Signature

```sql
BOOLEAN ST_IsPolygonCCW (geom GEOMETRY)
```

#### Description

Returns true if the exterior ring of a polygon is counter-clockwise

----

### ST_IsPolygonCW


#### Signature

```sql
BOOLEAN ST_IsPolygonCW (geom GEOMETRY)
```

#### Description

Returns true if the exterior ring of a polygon is clockwise

----

### ST_IsRing


#### Signature

```sql
BOOLEAN ST_IsRing (geom GEOMETRY)
```

#### Description

Returns true if the geometry is a ring (both ST_IsClosed and ST_IsSimple).

----

### ST_IsSimple


#### Signature

```sql
BOOLEAN ST_IsSimple (geom GEOMETRY)
```

#### Description

Returns true if the geometry is simple

----

### ST_IsValid


#### Signature

```sql
BOOLEAN ST_IsValid (geom GEOMETRY)
```

#### Description

Returns true if the geometry is valid

----

### ST_IsValidDetail


#### Signature

```sql
STRUCT("valid" BOOLEAN, reason VARCHAR, "location" GEOMETRY) ST_IsValidDetail (geom GEOMETRY)
```

#### Description

Returns a struct with validity info: {valid, reason, location}

#### Example

```sql
SELECT ST_IsValidDetail(ST_GeomFromText('POLYGON((0 0, 1 1, 1 0, 0 1, 0 0))'))
```

----

### ST_IsValidReason


#### Signature

```sql
VARCHAR ST_IsValidReason (geom GEOMETRY)
```

#### Description

Returns text explaining why a geometry is invalid, or 'Valid Geometry'

----

### ST_KNN


#### Signatures

```sql
BOOLEAN ST_KNN (geom1 GEOMETRY, geom2 GEOMETRY, k INTEGER)
BOOLEAN ST_KNN (geom1 GEOMETRY, geom2 GEOMETRY, k INTEGER, partition ANY)
```

#### Description

K-nearest-neighbor join predicate: matches each row of the `geom1` side with its `k` nearest rows of the `geom2` side.

`ST_KNN` is not a regular function. It is only valid as the condition of a `JOIN ... ON`, where the optimizer replaces the join with a dedicated `SPATIAL_KNN_JOIN` operator. Evaluating it anywhere else raises an error.

- **Arguments**: `geom1` is the probe side (every row of it is matched), `geom2` is the searched side, regardless of the order in which the two tables are written in the `FROM` clause. `k` must be a constant integer `>= 1`.
- **Distance**: planar euclidean distance between the two geometries, in the units of their coordinates, exactly as computed by `ST_Distance`. There is no geodesic mode: reproject longitude/latitude data to a metric CRS with `ST_Transform` first.
- **Exactness**: the result is the exact set of `k` nearest rows. An R-tree over the `geom2` side yields candidates by bounding box distance, and candidates are refined with the exact geometry distance until no closer row can exist. If fewer than `k` rows are available, all of them are returned. Ties at the `k`-th distance are broken arbitrarily.
- **Join types**: `INNER` and `LEFT`. Rows with a `NULL` or empty geometry never match; with a `LEFT JOIN` a probe row without any match is returned once, with `NULL` for the columns of the other side.
- **Other conditions**: a condition on a single table restricts the rows of that table *before* the search, exactly like filtering it in a subquery. A condition comparing both tables is not part of the search: an equality (`a.x = b.x`) raises an error, anything else only filters the k rows that were found (`INNER` joins only). To search within groups of the `geom2` side, use the `partition` argument.
- **Partitioning**: the optional `partition` argument is an expression over the `geom2` side. The search is then run independently within each distinct value of it, so each probe row is matched with its `k` nearest rows *per partition value* (`NULL` forms a partition of its own), while scanning the probe side only once.
- **Memory**: the `geom2` side is materialized and indexed in memory, so it should be the smaller of the two inputs.

#### Example

```sql
-- The 5 nearest hydrants of each building, with their distance (coordinates in a metric CRS)
SELECT b.id, h.id, ST_Distance(b.geom, h.geom) AS dist
FROM buildings b
JOIN hydrants h ON ST_KNN(b.geom, h.geom, 5);

-- Longitude/latitude input: project both sides to a metric CRS first
SELECT b.id, h.id
FROM (SELECT id, ST_Transform(geom, 'EPSG:4326', 'EPSG:3812', always_xy := true) AS geom FROM buildings) b
JOIN (SELECT id, ST_Transform(geom, 'EPSG:4326', 'EPSG:3812', always_xy := true) AS geom FROM hydrants) h
  ON ST_KNN(b.geom, h.geom, 5);

-- The nearest point of interest of every category for each building, in a single join
SELECT b.id, p.category, ST_Distance(b.geom, p.geom) AS dist
FROM buildings b
JOIN pois p ON ST_KNN(b.geom, p.geom, 1, p.category);
```

----

### ST_LargestEmptyCircle


#### Signature

```sql
GEOMETRY ST_LargestEmptyCircle (geom GEOMETRY, tolerance DOUBLE)
```

#### Description

Returns the largest empty circle within a geometry

----

### ST_Length


#### Signatures

```sql
DOUBLE ST_Length (geom GEOMETRY)
DOUBLE ST_Length (linestring LINESTRING_2D)
DOUBLE ST_Length (geog GEOG)
```

#### Description

Returns the length of the input line geometry

----

### ST_Length_Spheroid


#### Signatures

```sql
DOUBLE ST_Length_Spheroid (geom GEOMETRY)
DOUBLE ST_Length_Spheroid (line LINESTRING_2D)
```

#### Description

Returns the length of the input geometry in meters, using an ellipsoidal model of the earth

The input geometry is assumed to be in the [EPSG:4326](https://en.wikipedia.org/wiki/World_Geodetic_System) coordinate system (WGS84), with [latitude, longitude] axis order and the length is returned in meters. This function uses the [GeographicLib](https://geographiclib.sourceforge.io/) library, calculating the length using an ellipsoidal model of the earth. This is a highly accurate method for calculating the length of a line geometry taking the curvature of the earth into account, but is also the slowest.

Returns `0.0` for any geometry that is not a `LINESTRING`, `MULTILINESTRING` or `GEOMETRYCOLLECTION` containing line geometries.

----

### ST_LineFromEncodedPolyline


#### Signature

```sql
GEOMETRY ST_LineFromEncodedPolyline (encoded VARCHAR)
```

#### Description

Decodes a Google Encoded Polyline string into a linestring

#### Example

```sql
SELECT ST_AsText(ST_LineFromEncodedPolyline('_p~iF~ps|U_ulLnnqC_mqNvxq`@'))
```

----

### ST_LineFromMultiPoint


#### Signature

```sql
GEOMETRY ST_LineFromMultiPoint (multipoint GEOMETRY)
```

#### Description

Creates a linestring from the points of a multipoint geometry

#### Example

```sql
SELECT ST_AsText(ST_LineFromMultiPoint(ST_GeomFromText('MULTIPOINT(0 0, 1 1, 2 2)')))
```

----

### ST_LineInterpolatePoint


#### Signature

```sql
GEOMETRY ST_LineInterpolatePoint (line GEOMETRY, fraction DOUBLE)
```

#### Description

Returns a point interpolated along a line at a fraction of total 2D length.

----

### ST_LineInterpolatePoints


#### Signature

```sql
GEOMETRY ST_LineInterpolatePoints (line GEOMETRY, fraction DOUBLE, repeat BOOLEAN)
```

#### Description

Returns a multi-point interpolated along a line at a fraction of total 2D length.

if repeat is false, the result is a single point, (and equivalent to ST_LineInterpolatePoint),
otherwise, the result is a multi-point with points repeated at the fraction interval.

----

### ST_LineLocatePoint


#### Signature

```sql
DOUBLE ST_LineLocatePoint (line GEOMETRY, point GEOMETRY)
```

#### Description

Returns the location on a line closest to a point as a fraction of the total 2D length of the line.

----

### ST_LineMerge


#### Signatures

```sql
GEOMETRY ST_LineMerge (geom GEOMETRY)
GEOMETRY ST_LineMerge (geom GEOMETRY, preserve_direction BOOLEAN)
```

#### Description

"Merges" the input line geometry, optionally taking direction into account.

----

### ST_LineString2DFromWKB


#### Signature

```sql
LINESTRING_2D ST_LineString2DFromWKB (blob BLOB)
```

#### Description

Deserialize a LINESTRING_2D from a WKB encoded blob

----

### ST_LineSubstring


#### Signature

```sql
GEOMETRY ST_LineSubstring (line GEOMETRY, start_fraction DOUBLE, end_fraction DOUBLE)
```

#### Description

Returns a substring of a line between two fractions of total 2D length.

----

### ST_LocateAlong


#### Signatures

```sql
GEOMETRY ST_LocateAlong (line GEOMETRY, measure DOUBLE, offset DOUBLE)
GEOMETRY ST_LocateAlong (line GEOMETRY, measure DOUBLE)
```

#### Description

Returns a point or multi-point, containing the point(s) at the geometry with the given measure

For a LINESTRING, or MULTILINESTRING, the location is determined by interpolating between M values
For a POINT and MULTIPOINT, the point is returned if the measure matches the M value of the vertex, otherwise an empty geometry is returned
For a POLYGON, only the exterior ring is considered, and treated as a LINESTRING

If offset is provided, the resulting point(s) is offset by the given amount perpendicular to the line direction.

----

### ST_LocateBetween


#### Signatures

```sql
GEOMETRY ST_LocateBetween (line GEOMETRY, start_measure DOUBLE, end_measure DOUBLE, offset DOUBLE)
GEOMETRY ST_LocateBetween (line GEOMETRY, start_measure DOUBLE, end_measure DOUBLE)
```

#### Description

Returns a geometry or geometry collection created by filtering and interpolating vertices within a range of "M" values

Creates a geometry or geometry collection, containing the parts formed by vertices that have an "M" value within the "start_measure" and "end_measure" range

For LINESTRING or MULTILINESTRING, if a line segment would cross either the upper or lower bound, a vertex is added by interpolating the coordinates at the "intersection"
For a POINT and MULTIPOINT, the point is added to the collection if its vertex has an "M" value within the range, otherwise it is skipped
For a POLYGON, only the exterior ring is considered, and treated like a LINESTRING

If offset is provided, the resulting vertices are offset by the given amount perpendicular to the line direction.

----

### ST_LongestLine


#### Signature

```sql
GEOMETRY ST_LongestLine (geom1 GEOMETRY, geom2 GEOMETRY)
```

#### Description

Returns the longest line between two geometries (vertex-to-vertex)

#### Example

```sql
SELECT ST_AsText(ST_LongestLine(ST_Point(0, 0), ST_Point(1, 1)))
```

----

### ST_M


#### Signature

```sql
DOUBLE ST_M (geom GEOMETRY)
```

#### Description

Returns the M coordinate of a point geometry

#### Example

```sql
SELECT ST_M(ST_Point(1, 2, 3, 4))
```

----

### ST_MakeBox2D


#### Signature

```sql
BOX_2D ST_MakeBox2D (point1 GEOMETRY, point2 GEOMETRY)
```

#### Description

Create a BOX2D from two POINT geometries

#### Example

```sql
SELECT ST_MakeBox2D(ST_Point(0, 0), ST_Point(1, 1));
----
BOX(0 0, 1 1)
```

----

### ST_MakeEmptyRaster


#### Signatures

```sql
RASTER ST_MakeEmptyRaster (width INTEGER, height INTEGER, upperleftx DOUBLE, upperlefty DOUBLE, scalex DOUBLE, scaley DOUBLE, skewx DOUBLE, skewy DOUBLE)
RASTER ST_MakeEmptyRaster (width INTEGER, height INTEGER, upperleftx DOUBLE, upperlefty DOUBLE, scalex DOUBLE, scaley DOUBLE, skewx DOUBLE, skewy DOUBLE, srid INTEGER)
RASTER ST_MakeEmptyRaster (width INTEGER, height INTEGER, upperleftx DOUBLE, upperlefty DOUBLE, pixelsize DOUBLE)
RASTER ST_MakeEmptyRaster (rast RASTER)
```

#### Description

Creates a raster without bands.

`width` and `height` are in pixels, `upperleftx`/`upperlefty` are the world coordinates of the upper-left corner of the upper-left pixel, `scalex`/`scaley` the pixel size in world units (`scaley` is negative for north-up rasters) and `skewx`/`skewy` the rotation terms. `srid` is an EPSG code, 0 (the default) means no coordinate system. The `pixelsize` variant creates square, north-up pixels (`scalex = pixelsize`, `scaley = -pixelsize`). The single-argument variant copies the size, georeference and coordinate system of another raster.

A raster is a `RASTER` value: a GeoTIFF byte stream. Because GeoTIFF needs at least one band, a raster without bands is stored with a placeholder band that no function exposes.

#### Example

```sql
SELECT ST_MetaData(ST_MakeEmptyRaster(10, 5, 100, 200, 2, -2, 0, 0, 4326));
----
{'upperleftx': 100.0, 'upperlefty': 200.0, 'width': 10, 'height': 5, 'scalex': 2.0, 'scaley': -2.0, 'skewx': 0.0, 'skewy': 0.0, 'srid': 4326, 'numbands': 0}
```

----

### ST_MakeEnvelope


#### Signature

```sql
GEOMETRY ST_MakeEnvelope (min_x DOUBLE, min_y DOUBLE, max_x DOUBLE, max_y DOUBLE)
```

#### Description

Create a rectangular polygon from min/max coordinates

----

### ST_MakeLine


#### Signatures

```sql
GEOMETRY ST_MakeLine (geoms GEOMETRY[])
GEOMETRY ST_MakeLine (start GEOMETRY, end GEOMETRY)
```

#### Description

Create a LINESTRING from a list of POINT geometries

#### Example

```sql
SELECT ST_MakeLine([ST_Point(0, 0), ST_Point(1, 1)]);
----
LINESTRING(0 0, 1 1)
```

----

### ST_MakePoint


#### Signatures

```sql
POINT_2D ST_MakePoint (x DOUBLE, y DOUBLE)
POINT_3D ST_MakePoint (x DOUBLE, y DOUBLE, z DOUBLE)
POINT_4D ST_MakePoint (x DOUBLE, y DOUBLE, z DOUBLE, m DOUBLE)
```

#### Description

Creates a GEOMETRY point from an pair of floating point numbers.

For geodetic coordinate systems, x is typically the longitude value and y is the latitude value.

Note that ST_Point is equivalent. ST_MakePoint is provided for PostGIS compatibility.

#### Example

```sql
SELECT ST_AsText(ST_MakePoint(143.3, -24.2));
----
POINT (143.3 -24.2)
```

----

### ST_MakePolygon


#### Signatures

```sql
GEOMETRY ST_MakePolygon (shell GEOMETRY)
GEOMETRY ST_MakePolygon (shell GEOMETRY, holes GEOMETRY[])
```

#### Description

Create a POLYGON from a LINESTRING shell

#### Example

```sql
SELECT ST_MakePolygon(ST_LineString([ST_Point(0, 0), ST_Point(1, 0), ST_Point(1, 1), ST_Point(0, 0)]));
```

----

### ST_MakeValid


#### Signatures

```sql
GEOMETRY ST_MakeValid (geom GEOMETRY)
GEOMETRY ST_MakeValid (geom GEOMETRY, method VARCHAR)
GEOMETRY ST_MakeValid (geom GEOMETRY, method VARCHAR, keepCollapsed BOOLEAN)
```

#### Description

Returns a valid representation of the geometry

----

### ST_MapAlgebra


#### Signatures

```sql
RASTER ST_MapAlgebra (rast RASTER, nband INTEGER, pixeltype VARCHAR, expression VARCHAR)
RASTER ST_MapAlgebra (rast RASTER, nband INTEGER, pixeltype VARCHAR, expression VARCHAR, nodataval DOUBLE)
RASTER ST_MapAlgebra (rast RASTER, pixeltype VARCHAR, expression VARCHAR)
RASTER ST_MapAlgebra (rast RASTER, pixeltype VARCHAR, expression VARCHAR, nodataval DOUBLE)
RASTER ST_MapAlgebra (rast1 RASTER, nband1 INTEGER, rast2 RASTER, nband2 INTEGER, expression VARCHAR)
RASTER ST_MapAlgebra (rast1 RASTER, nband1 INTEGER, rast2 RASTER, nband2 INTEGER, expression VARCHAR, pixeltype VARCHAR)
RASTER ST_MapAlgebra (rast1 RASTER, nband1 INTEGER, rast2 RASTER, nband2 INTEGER, expression VARCHAR, pixeltype VARCHAR, extenttype VARCHAR)
RASTER ST_MapAlgebra (rast1 RASTER, nband1 INTEGER, rast2 RASTER, nband2 INTEGER, expression VARCHAR, pixeltype VARCHAR, extenttype VARCHAR, nodata1expr VARCHAR)
RASTER ST_MapAlgebra (rast1 RASTER, nband1 INTEGER, rast2 RASTER, nband2 INTEGER, expression VARCHAR, pixeltype VARCHAR, extenttype VARCHAR, nodata1expr VARCHAR, nodata2expr VARCHAR)
RASTER ST_MapAlgebra (rast1 RASTER, nband1 INTEGER, rast2 RASTER, nband2 INTEGER, expression VARCHAR, pixeltype VARCHAR, extenttype VARCHAR, nodata1expr VARCHAR, nodata2expr VARCHAR, nodatanodataval DOUBLE)
RASTER ST_MapAlgebra (rast1 RASTER, rast2 RASTER, expression VARCHAR)
RASTER ST_MapAlgebra (rast1 RASTER, rast2 RASTER, expression VARCHAR, pixeltype VARCHAR)
RASTER ST_MapAlgebra (rast1 RASTER, rast2 RASTER, expression VARCHAR, pixeltype VARCHAR, extenttype VARCHAR)
RASTER ST_MapAlgebra (rast1 RASTER, rast2 RASTER, expression VARCHAR, pixeltype VARCHAR, extenttype VARCHAR, nodata1expr VARCHAR)
RASTER ST_MapAlgebra (rast1 RASTER, rast2 RASTER, expression VARCHAR, pixeltype VARCHAR, extenttype VARCHAR, nodata1expr VARCHAR, nodata2expr VARCHAR)
RASTER ST_MapAlgebra (rast1 RASTER, rast2 RASTER, expression VARCHAR, pixeltype VARCHAR, extenttype VARCHAR, nodata1expr VARCHAR, nodata2expr VARCHAR, nodatanodataval DOUBLE)
```

#### Description

Computes a new single-band raster pixel by pixel from one band, or from one band of each of two rasters, with a SQL expression.

One raster: `expression` is evaluated for every pixel of band `nband` (1-based, default 1) that is not NODATA. It may use `[rast]` (or `[rast.val]`, the pixel value, a DOUBLE) and `[rast.x]` / `[rast.y]` (the 1-based column and row, INTEGER). Pixels that are NODATA in the input, and pixels for which the expression is NULL, are NODATA in the result. The NODATA value of the result is `nodataval`, by default the one of the input band, or the smallest value of the pixel type if there is none and one is needed.

Two rasters: the rasters must have the same alignment (see ST_SameAlignment and ST_Resample). `expression` may use `[rast1]`, `[rast1.x]`, `[rast1.y]`, `[rast2]`, `[rast2.x]` and `[rast2.y]`, and is evaluated where both rasters have a value. `extenttype` is the extent of the result: `INTERSECTION` (the default), `UNION`, `FIRST` or `SECOND`. Where only the second raster has a value, the result is `nodata1expr` (an expression, NODATA if omitted); where only the first one has a value, `nodata2expr`; where neither has, the constant `nodatanodataval`. Returns NULL if the extent is empty (PostGIS returns an empty raster).

`pixeltype` is the pixel type of the result; NULL means the type of the (first) input band. Results are rounded and clamped to the pixel type.

The expression must be a constant string. It is parsed and bound once per query by DuckDB's own parser and binder and evaluated vectorised over the pixels, so every DuckDB scalar function and operator is available and the syntax and semantics are those of DuckDB, not of PostgreSQL. Subqueries, window functions and aggregates are not allowed. The callback (`regprocedure`) variants of PostGIS are not available.

#### Example

```sql
SELECT ST_DumpValues(ST_MapAlgebra(ST_AddBand(ST_MakeEmptyRaster(3, 2, 0, 0, 1), '8BUI', 10), 1, '16BSI', '[rast] * 2 + [rast.x] - [rast.y]'));
----
[[20.0, 21.0, 22.0], [19.0, 20.0, 21.0]]
```

----

### ST_MaxDistance


#### Signature

```sql
DOUBLE ST_MaxDistance (geom1 GEOMETRY, geom2 GEOMETRY)
```

#### Description

Returns the maximum distance between two geometries

----

### ST_MaximumInscribedCircle


#### Signatures

```sql
STRUCT(center GEOMETRY, nearest GEOMETRY, radius DOUBLE) ST_MaximumInscribedCircle (geom GEOMETRY)
STRUCT(center GEOMETRY, nearest GEOMETRY, radius DOUBLE) ST_MaximumInscribedCircle (geom GEOMETRY, tolerance DOUBLE)
```

#### Description

Returns the maximum inscribed circle of the input geometry, optionally with a tolerance.

By default, the tolerance is computed as `max(width, height) / 1000`.
The return value is a struct with the center of the circle, the nearest point to the center on the boundary of the geometry, and the radius of the circle.

#### Example

```sql
-- Find the maximum inscribed circle of a square
SELECT ST_MaximumInscribedCircle(
    ST_GeomFromText('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))')
);
----
{'center': POINT (5 5), 'nearest': POINT (5 0), 'radius': 5.0}
```

----

### ST_MemSize


#### Signature

```sql
INTEGER ST_MemSize (geom GEOMETRY)
```

#### Description

Returns the memory size of a geometry in bytes

#### Example

```sql
SELECT ST_MemSize(ST_Point(1, 2))
```

----

### ST_MetaData


#### Signature

```sql
STRUCT(upperleftx DOUBLE, upperlefty DOUBLE, width INTEGER, height INTEGER, scalex DOUBLE, scaley DOUBLE, skewx DOUBLE, skewy DOUBLE, srid INTEGER, numbands INTEGER) ST_MetaData (rast RASTER)
```

#### Description

Returns the size, georeference, SRID (0 when the coordinate system is not an EPSG code) and number of bands of the raster as a struct.

#### Example

```sql
SELECT ST_MetaData(ST_MakeEmptyRaster(10, 5, 100, 200, 2, -2, 0, 0, 4326));
----
{'upperleftx': 100.0, 'upperlefty': 200.0, 'width': 10, 'height': 5, 'scalex': 2.0, 'scaley': -2.0, 'skewx': 0.0, 'skewy': 0.0, 'srid': 4326, 'numbands': 0}
```

----

### ST_MinConvexHull


#### Signatures

```sql
GEOMETRY ST_MinConvexHull (rast RASTER)
GEOMETRY ST_MinConvexHull (rast RASTER, nband INTEGER)
```

#### Description

Returns the outline of the smallest pixel window that contains every pixel that is not NODATA, in band `nband` (1-based) or in any band when omitted or NULL. Returns NULL if all pixels are NODATA.

#### Example

```sql
SELECT ST_MinConvexHull(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(4, 4, 0, 0, 1), '8BUI', 0, 0), 2, 2, 9));
----
POLYGON ((1 -1, 2 -1, 2 -2, 1 -2, 1 -1))
```

----

### ST_MinimumBoundingCircle


#### Signature

```sql
GEOMETRY ST_MinimumBoundingCircle (geom GEOMETRY)
```

#### Description

Returns the minimum bounding circle of a geometry

----

### ST_MinimumClearance


#### Signature

```sql
DOUBLE ST_MinimumClearance (geom GEOMETRY)
```

#### Description

Returns the minimum clearance of a geometry

----

### ST_MinimumClearanceLine


#### Signature

```sql
GEOMETRY ST_MinimumClearanceLine (geom GEOMETRY)
```

#### Description

Returns the line spanning the minimum clearance

----

### ST_MinimumRotatedRectangle


#### Signature

```sql
GEOMETRY ST_MinimumRotatedRectangle (geom GEOMETRY)
```

#### Description

Returns the minimum rotated rectangle that bounds the input geometry, finding the surrounding box that has the lowest area by using a rotated rectangle, rather than taking the lowest and highest coordinate values as per ST_Envelope().

----

### ST_MMax


#### Signature

```sql
DOUBLE ST_MMax (geom GEOMETRY)
```

#### Description

Returns the maximum M coordinate of a geometry

#### Example

```sql
SELECT ST_MMax(ST_Point(1, 2, 3, 4))
```

----

### ST_MMin


#### Signature

```sql
DOUBLE ST_MMin (geom GEOMETRY)
```

#### Description

Returns the minimum M coordinate of a geometry

#### Example

```sql
SELECT ST_MMin(ST_Point(1, 2, 3, 4))
```

----

### ST_Multi


#### Signature

```sql
GEOMETRY ST_Multi (geom GEOMETRY)
```

#### Description

Turns a single geometry into a multi geometry.

If the geometry is already a multi geometry, it is returned as is.

#### Example

```sql
SELECT ST_Multi(ST_GeomFromText('POINT(1 2)'));
----
MULTIPOINT (1 2)

SELECT ST_Multi(ST_GeomFromText('LINESTRING(1 1, 2 2)'));
----
MULTILINESTRING ((1 1, 2 2))

SELECT ST_Multi(ST_GeomFromText('POLYGON((0 0, 0 1, 1 1, 1 0, 0 0))'));
----
MULTIPOLYGON (((0 0, 0 1, 1 1, 1 0, 0 0)))
```

----

### ST_NDims


#### Signature

```sql
INTEGER ST_NDims (geom GEOMETRY)
```

#### Description

Returns the topological dimension of a geometry

----

### ST_NearestValue


#### Signatures

```sql
DOUBLE ST_NearestValue (rast RASTER, pt GEOMETRY)
DOUBLE ST_NearestValue (rast RASTER, pt GEOMETRY, exclude_nodata_value BOOLEAN)
DOUBLE ST_NearestValue (rast RASTER, band INTEGER, pt GEOMETRY)
DOUBLE ST_NearestValue (rast RASTER, band INTEGER, pt GEOMETRY, exclude_nodata_value BOOLEAN)
DOUBLE ST_NearestValue (rast RASTER, columnx INTEGER, rowy INTEGER)
DOUBLE ST_NearestValue (rast RASTER, columnx INTEGER, rowy INTEGER, exclude_nodata_value BOOLEAN)
DOUBLE ST_NearestValue (rast RASTER, band INTEGER, columnx INTEGER, rowy INTEGER)
DOUBLE ST_NearestValue (rast RASTER, band INTEGER, columnx INTEGER, rowy INTEGER, exclude_nodata_value BOOLEAN)
```

#### Description

Returns the value of the pixel at a point or at a 1-based column and row if it is not NODATA, otherwise the value of the nearest pixel that is not NODATA.

The distance is measured in world units from the point (or the centre of the given pixel) to the pixel centres. The location may be outside of the raster. Returns NULL if the band (1-based, default 1) has no such pixel.

#### Example

```sql
SELECT ST_NearestValue(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(4, 4, 0, 0, 1), '8BUI', 0, 0), 4, 4, 9), 1, 1);
----
9.0
```

----

### ST_NGeometries


#### Signature

```sql
INTEGER ST_NGeometries (geom GEOMETRY)
```

#### Description

Returns the number of component geometries in a collection geometry.
If the input geometry is not a collection, this function returns 0 or 1 depending on if the geometry is empty or not.

----

### ST_NInteriorRings


#### Signatures

```sql
INTEGER ST_NInteriorRings (geom GEOMETRY)
INTEGER ST_NInteriorRings (polygon POLYGON_2D)
```

#### Description

Returns the number of interior rings of a polygon

----

### ST_Node


#### Signature

```sql
GEOMETRY ST_Node (geom GEOMETRY)
```

#### Description

Returns a "noded" MultiLinestring, produced by combining a collection of input linestrings and adding additional vertices where they intersect.

#### Example

```sql
-- Create a noded multilinestring from two intersecting lines
SELECT ST_Node(
    ST_GeomFromText('MULTILINESTRING((0 0, 2 2), (0 2, 2 0))')
);
----
MULTILINESTRING ((0 0, 1 1), (1 1, 2 2), (0 2, 1 1), (1 1, 2 0))
```

----

### ST_Normalize


#### Signature

```sql
GEOMETRY ST_Normalize (geom GEOMETRY)
```

#### Description

Returns the "normalized" representation of the geometry

----

### ST_NotSameAlignmentReason


#### Signature

```sql
VARCHAR ST_NotSameAlignmentReason (rast1 RASTER, rast2 RASTER)
```

#### Description

Returns the reason why two rasters are not aligned (see ST_SameAlignment), or 'The rasters are aligned'.

#### Example

```sql
SELECT ST_NotSameAlignmentReason(ST_MakeEmptyRaster(2, 2, 0, 0, 1), ST_MakeEmptyRaster(2, 2, 0.5, 0, 1));
----
The rasters (pixel corner coordinates) are not aligned
```

----

### ST_NPoints


#### Signatures

```sql
UINTEGER ST_NPoints (geom GEOMETRY)
UBIGINT ST_NPoints (point POINT_2D)
UBIGINT ST_NPoints (linestring LINESTRING_2D)
UBIGINT ST_NPoints (polygon POLYGON_2D)
UBIGINT ST_NPoints (box BOX_2D)
```

#### Description

Returns the number of vertices within a geometry

----

### ST_NRings


#### Signature

```sql
INTEGER ST_NRings (geom GEOMETRY)
```

#### Description

Returns the number of rings in a polygon (exterior + interior)

----

### ST_NumBands


#### Signature

```sql
INTEGER ST_NumBands (rast RASTER)
```

#### Description

Returns the number of bands of the raster.

#### Example

```sql
SELECT ST_NumBands(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '8BUI'));
----
1
```

----

### ST_NumGeometries


#### Signature

```sql
INTEGER ST_NumGeometries (geom GEOMETRY)
```

#### Description

Returns the number of component geometries in a collection geometry.
If the input geometry is not a collection, this function returns 0 or 1 depending on if the geometry is empty or not.

----

### ST_NumInteriorRings


#### Signatures

```sql
INTEGER ST_NumInteriorRings (geom GEOMETRY)
INTEGER ST_NumInteriorRings (polygon POLYGON_2D)
```

#### Description

Returns the number of interior rings of a polygon

----

### ST_NumPoints


#### Signatures

```sql
UINTEGER ST_NumPoints (geom GEOMETRY)
UBIGINT ST_NumPoints (point POINT_2D)
UBIGINT ST_NumPoints (linestring LINESTRING_2D)
UBIGINT ST_NumPoints (polygon POLYGON_2D)
UBIGINT ST_NumPoints (box BOX_2D)
```

#### Description

Returns the number of vertices within a geometry

----

### ST_OffsetCurve


#### Signature

```sql
GEOMETRY ST_OffsetCurve (geom GEOMETRY, distance DOUBLE)
```

#### Description

Returns an offset curve from a linestring

----

### ST_OrderingEquals


#### Signature

```sql
BOOLEAN ST_OrderingEquals (geom1 GEOMETRY, geom2 GEOMETRY)
```

#### Description

Returns true if two geometries are exactly equal (same vertex order)

----

### ST_Overlaps


#### Signatures

```sql
BOOLEAN ST_Overlaps (geom1 GEOMETRY, geom2 GEOMETRY)
BOOLEAN ST_Overlaps (rast1 RASTER, rast2 RASTER)
BOOLEAN ST_Overlaps (rast1 RASTER, nband1 INTEGER, rast2 RASTER, nband2 INTEGER)
```

#### Description

Returns true if the geometries overlap

----

### ST_Perimeter


#### Signatures

```sql
DOUBLE ST_Perimeter (geom GEOMETRY)
DOUBLE ST_Perimeter (polygon POLYGON_2D)
DOUBLE ST_Perimeter (box BOX_2D)
DOUBLE ST_Perimeter (geog GEOG)
```

#### Description

Returns the length of the perimeter of the geometry

----

### ST_Perimeter_Spheroid


#### Signatures

```sql
DOUBLE ST_Perimeter_Spheroid (geom GEOMETRY)
DOUBLE ST_Perimeter_Spheroid (poly POLYGON_2D)
```

#### Description

Returns the length of the perimeter in meters using an ellipsoidal model of the earths surface

The input geometry is assumed to be in the [EPSG:4326](https://en.wikipedia.org/wiki/World_Geodetic_System) coordinate system (WGS84), with [latitude, longitude] axis order and the length is returned in meters. This function uses the [GeographicLib](https://geographiclib.sourceforge.io/) library, calculating the perimeter using an ellipsoidal model of the earth. This is a highly accurate method for calculating the perimeter of a polygon taking the curvature of the earth into account, but is also the slowest.

Returns `0.0` for any geometry that is not a `POLYGON`, `MULTIPOLYGON` or `GEOMETRYCOLLECTION` containing polygon geometries.

----

### ST_PixelAsCentroid


#### Signature

```sql
GEOMETRY ST_PixelAsCentroid (rast RASTER, x INTEGER, y INTEGER)
```

#### Description

Returns the centre of a pixel (1-based column `x` and row `y`) as a point.

#### Example

```sql
SELECT ST_PixelAsCentroid(ST_MakeEmptyRaster(10, 5, 100, 200, 2, -2, 0, 0), 2, 3);
----
POINT (103 195)
```

----

### ST_PixelAsCentroids


#### Signatures

```sql
STRUCT(geom GEOMETRY, val DOUBLE, x INTEGER, y INTEGER)[] ST_PixelAsCentroids (rast RASTER)
STRUCT(geom GEOMETRY, val DOUBLE, x INTEGER, y INTEGER)[] ST_PixelAsCentroids (rast RASTER, band INTEGER)
STRUCT(geom GEOMETRY, val DOUBLE, x INTEGER, y INTEGER)[] ST_PixelAsCentroids (rast RASTER, band INTEGER, exclude_nodata_value BOOLEAN)
```

#### Description

Returns one entry per pixel of a band as a list of structs `(geom, val, x, y)`: the pixel's centre as a point, its value and its 1-based column and row.

`band` is 1-based and defaults to 1. NODATA pixels are left out unless `exclude_nodata_value` is false. PostGIS returns a set of rows; use `UNNEST(..., recursive := true)` to get the same shape.

#### Example

```sql
SELECT UNNEST(ST_PixelAsCentroids(ST_AddBand(ST_MakeEmptyRaster(2, 1, 0, 0, 1), '8BUI', 7)), recursive := true);
----
POINT (0.5 -0.5)    7.0    1    1
POINT (1.5 -0.5)    7.0    2    1
```

----

### ST_PixelAsPoint


#### Signature

```sql
GEOMETRY ST_PixelAsPoint (rast RASTER, x INTEGER, y INTEGER)
```

#### Description

Returns the upper-left corner of a pixel (1-based column `x` and row `y`) as a point.

#### Example

```sql
SELECT ST_PixelAsPoint(ST_MakeEmptyRaster(10, 5, 100, 200, 2, -2, 0, 0), 2, 3);
----
POINT (102 196)
```

----

### ST_PixelAsPoints


#### Signatures

```sql
STRUCT(geom GEOMETRY, val DOUBLE, x INTEGER, y INTEGER)[] ST_PixelAsPoints (rast RASTER)
STRUCT(geom GEOMETRY, val DOUBLE, x INTEGER, y INTEGER)[] ST_PixelAsPoints (rast RASTER, band INTEGER)
STRUCT(geom GEOMETRY, val DOUBLE, x INTEGER, y INTEGER)[] ST_PixelAsPoints (rast RASTER, band INTEGER, exclude_nodata_value BOOLEAN)
```

#### Description

Returns one entry per pixel of a band as a list of structs `(geom, val, x, y)`: the pixel's upper-left corner as a point, its value and its 1-based column and row.

`band` is 1-based and defaults to 1. NODATA pixels are left out unless `exclude_nodata_value` is false. PostGIS returns a set of rows; use `UNNEST(..., recursive := true)` to get the same shape.

#### Example

```sql
SELECT UNNEST(ST_PixelAsPoints(ST_AddBand(ST_MakeEmptyRaster(2, 1, 0, 0, 1), '8BUI', 7)), recursive := true);
----
POINT (0 0)    7.0    1    1
POINT (1 0)    7.0    2    1
```

----

### ST_PixelAsPolygon


#### Signature

```sql
GEOMETRY ST_PixelAsPolygon (rast RASTER, x INTEGER, y INTEGER)
```

#### Description

Returns the outline of a pixel (1-based column `x` and row `y`) as a polygon.

#### Example

```sql
SELECT ST_PixelAsPolygon(ST_MakeEmptyRaster(10, 5, 100, 200, 2, -2, 0, 0), 2, 3);
----
POLYGON ((102 196, 104 196, 104 194, 102 194, 102 196))
```

----

### ST_PixelAsPolygons


#### Signatures

```sql
STRUCT(geom GEOMETRY, val DOUBLE, x INTEGER, y INTEGER)[] ST_PixelAsPolygons (rast RASTER)
STRUCT(geom GEOMETRY, val DOUBLE, x INTEGER, y INTEGER)[] ST_PixelAsPolygons (rast RASTER, band INTEGER)
STRUCT(geom GEOMETRY, val DOUBLE, x INTEGER, y INTEGER)[] ST_PixelAsPolygons (rast RASTER, band INTEGER, exclude_nodata_value BOOLEAN)
```

#### Description

Returns one entry per pixel of a band as a list of structs `(geom, val, x, y)`: the pixel's outline as a polygon, its value and its 1-based column and row.

`band` is 1-based and defaults to 1. NODATA pixels are left out unless `exclude_nodata_value` is false. PostGIS returns a set of rows; use `UNNEST(..., recursive := true)` to get the same shape.

#### Example

```sql
SELECT UNNEST(ST_PixelAsPolygons(ST_AddBand(ST_MakeEmptyRaster(2, 1, 0, 0, 1), '8BUI', 7)), recursive := true);
----
POLYGON ((0 0, 1 0, 1 -1, 0 -1, 0 0))    7.0    1    1
POLYGON ((1 0, 2 0, 2 -1, 1 -1, 1 0))    7.0    2    1
```

----

### ST_PixelHeight


#### Signature

```sql
DOUBLE ST_PixelHeight (rast RASTER)
```

#### Description

Returns the height of a pixel in world units, taking the skew into account: `sqrt(scaley^2 + skewx^2)`.

#### Example

```sql
SELECT ST_PixelHeight(ST_MakeEmptyRaster(10, 5, 0, 0, 2, -3, 4, 0));
----
5.0
```

----

### ST_PixelWidth


#### Signature

```sql
DOUBLE ST_PixelWidth (rast RASTER)
```

#### Description

Returns the width of a pixel in world units, taking the skew into account: `sqrt(scalex^2 + skewy^2)`.

#### Example

```sql
SELECT ST_PixelWidth(ST_MakeEmptyRaster(10, 5, 0, 0, 3, -2, 0, 4));
----
5.0
```

----

### ST_Point


#### Signature

```sql
GEOMETRY ST_Point (x DOUBLE, y DOUBLE)
```

#### Description

Creates a GEOMETRY point

----

### ST_Point2D


#### Signature

```sql
POINT_2D ST_Point2D (x DOUBLE, y DOUBLE)
```

#### Description

Creates a POINT_2D

----

### ST_Point2DFromWKB


#### Signature

```sql
POINT_2D ST_Point2DFromWKB (blob BLOB)
```

#### Description

Deserialize a POINT_2D from a WKB encoded blob

----

### ST_Point3D


#### Signature

```sql
POINT_3D ST_Point3D (x DOUBLE, y DOUBLE, z DOUBLE)
```

#### Description

Creates a POINT_3D

----

### ST_Point4D


#### Signature

```sql
POINT_4D ST_Point4D (x DOUBLE, y DOUBLE, z DOUBLE, m DOUBLE)
```

#### Description

Creates a POINT_4D

----

### ST_PointN


#### Signatures

```sql
GEOMETRY ST_PointN (geom GEOMETRY, index INTEGER)
POINT_2D ST_PointN (linestring LINESTRING_2D, index INTEGER)
```

#### Description

Returns the n'th vertex from the input geometry as a point geometry

----

### ST_PointOnSurface


#### Signature

```sql
GEOMETRY ST_PointOnSurface (geom GEOMETRY)
```

#### Description

Returns a point guaranteed to lie on the surface of the geometry

----

### ST_Points


#### Signature

```sql
GEOMETRY ST_Points (geom GEOMETRY)
```

#### Description

Collects all the vertices in the geometry into a MULTIPOINT

#### Example

```sql
SELECT ST_Points('LINESTRING(1 1, 2 2)'::GEOMETRY);
----
MULTIPOINT (1 1, 2 2)

SELECT ST_Points('MULTIPOLYGON Z EMPTY'::GEOMETRY);
----
MULTIPOINT Z EMPTY
```

----

### ST_Polygon


#### Signatures

```sql
GEOMETRY ST_Polygon (line GEOMETRY)
GEOMETRY ST_Polygon (rast RASTER)
GEOMETRY ST_Polygon (rast RASTER, band INTEGER)
```

#### Description

Creates a polygon from a closed linestring

#### Example

```sql
SELECT ST_AsText(ST_Polygon(ST_GeomFromText('LINESTRING(0 0, 1 0, 1 1, 0 1, 0 0)')))
```

----

### ST_Polygon2DFromWKB


#### Signature

```sql
POLYGON_2D ST_Polygon2DFromWKB (blob BLOB)
```

#### Description

Deserialize a POLYGON_2D from a WKB encoded blob

----

### ST_Polygonize


#### Signature

```sql
GEOMETRY ST_Polygonize (geometries GEOMETRY[])
```

#### Description

Returns a polygonized representation of the input geometries

#### Example

```sql
-- Create a polygon from a closed linestring ring
SELECT ST_Polygonize([
    ST_GeomFromText('LINESTRING(0 0, 0 10, 10 10, 10 0, 0 0)')
]);
---
GEOMETRYCOLLECTION (POLYGON ((0 0, 0 10, 10 10, 10 0, 0 0)))
```

----

### ST_Project


#### Signatures

```sql
GEOMETRY ST_Project (point GEOMETRY, distance DOUBLE, azimuth DOUBLE)
GEOG ST_Project (origin GEOG, distance DOUBLE, azimuth DOUBLE)
```

#### Description

Projects a point along the geodesic by a distance (meters) and azimuth (radians)

#### Example

```sql
SELECT ST_AsText(ST_Project(ST_Point(0, 0), 100000, 0))
```

----

### ST_QuadKey


#### Signatures

```sql
VARCHAR ST_QuadKey (longitude DOUBLE, latitude DOUBLE, level INTEGER)
VARCHAR ST_QuadKey (point GEOMETRY, level INTEGER)
```

#### Description

Compute the [quadkey](https://learn.microsoft.com/en-us/bingmaps/articles/bing-maps-tile-system) for a given lon/lat point at a given level.
Note that the parameter order is __longitude__, __latitude__.

`level` has to be between 1 and 23, inclusive.

The input coordinates will be clamped to the lon/lat bounds of the earth (longitude between -180 and 180, latitude between -85.05112878 and 85.05112878).

The geometry overload throws an error if the input geometry is not a `POINT`

#### Example

```sql
SELECT ST_QuadKey(ST_Point(11.08, 49.45), 10);
----
1333203202
```

----

### ST_Quantile


#### Signatures

```sql
STRUCT(quantile DOUBLE, "value" DOUBLE)[] ST_Quantile (rast RASTER)
STRUCT(quantile DOUBLE, "value" DOUBLE)[] ST_Quantile (rast RASTER, nband INTEGER)
STRUCT(quantile DOUBLE, "value" DOUBLE)[] ST_Quantile (rast RASTER, nband INTEGER, exclude_nodata_value BOOLEAN)
STRUCT(quantile DOUBLE, "value" DOUBLE)[] ST_Quantile (rast RASTER, nband INTEGER, exclude_nodata_value BOOLEAN, quantiles DOUBLE[])
STRUCT(quantile DOUBLE, "value" DOUBLE)[] ST_Quantile (rast RASTER, nband INTEGER, quantiles DOUBLE[])
STRUCT(quantile DOUBLE, "value" DOUBLE)[] ST_Quantile (rast RASTER, quantiles DOUBLE[])
DOUBLE ST_Quantile (rast RASTER, nband INTEGER, exclude_nodata_value BOOLEAN, quantile DOUBLE)
DOUBLE ST_Quantile (rast RASTER, nband INTEGER, quantile DOUBLE)
DOUBLE ST_Quantile (rast RASTER, exclude_nodata_value BOOLEAN, quantile DOUBLE)
DOUBLE ST_Quantile (rast RASTER, quantile DOUBLE)
```

#### Description

Returns quantiles of the pixel values of a band.

With a single `quantile` between 0 and 1 the result is its value. With a list of `quantiles`, or without any (the quartiles 0, 0.25, 0.5, 0.75 and 1), the result is a list of structs `(quantile, value)` in ascending order; PostGIS returns a set of rows. Quantiles are interpolated linearly between the closest ranks, like `quantile_cont`. `nband` is 1-based and defaults to 1; NODATA pixels are left out unless `exclude_nodata_value` is false. The value is NULL when no pixel is counted.

#### Example

```sql
SELECT ST_Quantile(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '8BUI', 1), 1, 1, 5), 0.5);
----
1.0
```

----

### ST_QuantizeCoordinates


#### Signature

```sql
GEOMETRY ST_QuantizeCoordinates (geom GEOMETRY, precision INTEGER)
```

#### Description

Rounds all coordinates to the given number of decimal places

#### Example

```sql
SELECT ST_AsText(ST_QuantizeCoordinates(ST_Point(1.23456, 2.78901), 2))
```

----

### ST_RasterToWorldCoord


#### Signature

```sql
STRUCT(longitude DOUBLE, latitude DOUBLE) ST_RasterToWorldCoord (rast RASTER, columnx INTEGER, rowy INTEGER)
```

#### Description

Returns the world coordinates of the upper-left corner of a pixel as a struct. Pixel columns and rows are numbered from 1 and may lie outside of the raster.

#### Example

```sql
SELECT ST_RasterToWorldCoord(ST_MakeEmptyRaster(10, 5, 100, 200, 2, -2, 0, 0), 2, 3);
----
{'longitude': 102.0, 'latitude': 196.0}
```

----

### ST_RasterToWorldCoordX


#### Signatures

```sql
DOUBLE ST_RasterToWorldCoordX (rast RASTER, xr INTEGER, yr INTEGER)
DOUBLE ST_RasterToWorldCoordX (rast RASTER, xr INTEGER)
```

#### Description

Returns the world X coordinate of the upper-left corner of a pixel (columns and rows numbered from 1). The row may be omitted if the raster is not skewed.

#### Example

```sql
SELECT ST_RasterToWorldCoordX(ST_MakeEmptyRaster(10, 5, 100, 200, 2, -2, 0, 0), 2);
----
102.0
```

----

### ST_RasterToWorldCoordY


#### Signatures

```sql
DOUBLE ST_RasterToWorldCoordY (rast RASTER, xr INTEGER, yr INTEGER)
DOUBLE ST_RasterToWorldCoordY (rast RASTER, yr INTEGER)
```

#### Description

Returns the world Y coordinate of the upper-left corner of a pixel (columns and rows numbered from 1). The column may be omitted if the raster is not skewed.

#### Example

```sql
SELECT ST_RasterToWorldCoordY(ST_MakeEmptyRaster(10, 5, 100, 200, 2, -2, 0, 0), 3);
----
196.0
```

----

### ST_Reclass


#### Signatures

```sql
RASTER ST_Reclass (rast RASTER, nband INTEGER, reclassexpr VARCHAR, pixeltype VARCHAR)
RASTER ST_Reclass (rast RASTER, nband INTEGER, reclassexpr VARCHAR, pixeltype VARCHAR, nodataval DOUBLE)
RASTER ST_Reclass (rast RASTER, reclassexpr VARCHAR, pixeltype VARCHAR)
```

#### Description

Returns the raster with the values of band `nband` (1-based, default 1) mapped to new values, stored with a new pixel type.

`reclassexpr` is a comma-separated list of `range:map_range` entries. A range is a single value or `min-max`; its values are mapped linearly onto `map_range` and the first matching entry wins. The minimum is included unless the range starts with `(`, the maximum is excluded unless the range ends with `]`: `[0-100]` is 0 <= x <= 100, `(0-100]` is 0 < x <= 100, `0-100` and `[0-100)` are 0 <= x < 100. Negative numbers are written as is: `-10--5:1-2`. For integer pixel types the result is rounded.

Pixels that match no entry, and NODATA pixels, are set to `nodataval`, which becomes the NODATA value of the band. Without `nodataval` the band has no NODATA value, unmatched pixels are 0 and NODATA pixels are reclassified like any other value. The other bands are unchanged and must have the same pixel type as the result. The `reclassarg[]` variant of PostGIS is not available: nest calls instead.

#### Example

```sql
SELECT ST_DumpValues(ST_Reclass(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(2, 1, 0, 0, 1), '32BF', 50), 2, 1, 150), 1, '[0-100]:1-11, (100-200]:20', '8BUI', 0));
----
[[6.0, 20.0]]
```

----

### ST_ReducePrecision


#### Signature

```sql
GEOMETRY ST_ReducePrecision (geom GEOMETRY, precision DOUBLE)
```

#### Description

Returns the geometry with all vertices reduced to the given precision

----

### ST_Relate


#### Signature

```sql
VARCHAR ST_Relate (geom1 GEOMETRY, geom2 GEOMETRY)
```

#### Description

Returns the DE-9IM intersection matrix string

----

### ST_RelateMatch


#### Signature

```sql
BOOLEAN ST_RelateMatch (matrix VARCHAR, pattern VARCHAR)
```

#### Description

Tests if a DE-9IM matrix string matches a DE-9IM pattern

#### Example

```sql
SELECT ST_RelateMatch('FF2FF1FF2', 'FF*FF****')
```

----

### ST_RemovePoint


#### Signature

```sql
GEOMETRY ST_RemovePoint (line GEOMETRY, position INTEGER)
```

#### Description

Removes a point from a linestring (0-indexed, negative from end)

#### Example

```sql
SELECT ST_AsText(ST_RemovePoint(ST_GeomFromText('LINESTRING(0 0, 1 1, 2 2)'), 1))
```

----

### ST_RemoveRepeatedPoints


#### Signatures

```sql
LINESTRING_2D ST_RemoveRepeatedPoints (line LINESTRING_2D)
LINESTRING_2D ST_RemoveRepeatedPoints (line LINESTRING_2D, tolerance DOUBLE)
GEOMETRY ST_RemoveRepeatedPoints (geom GEOMETRY)
GEOMETRY ST_RemoveRepeatedPoints (geom GEOMETRY, tolerance DOUBLE)
```

#### Description

Remove repeated points from a LINESTRING.

----

### ST_Resample


#### Signatures

```sql
RASTER ST_Resample (rast RASTER, scalex DOUBLE, scaley DOUBLE)
RASTER ST_Resample (rast RASTER, scalex DOUBLE, scaley DOUBLE, gridx DOUBLE)
RASTER ST_Resample (rast RASTER, scalex DOUBLE, scaley DOUBLE, gridx DOUBLE, gridy DOUBLE)
RASTER ST_Resample (rast RASTER, scalex DOUBLE, scaley DOUBLE, gridx DOUBLE, gridy DOUBLE, skewx DOUBLE)
RASTER ST_Resample (rast RASTER, scalex DOUBLE, scaley DOUBLE, gridx DOUBLE, gridy DOUBLE, skewx DOUBLE, skewy DOUBLE)
RASTER ST_Resample (rast RASTER, scalex DOUBLE, scaley DOUBLE, gridx DOUBLE, gridy DOUBLE, skewx DOUBLE, skewy DOUBLE, algorithm VARCHAR)
RASTER ST_Resample (rast RASTER, scalex DOUBLE, scaley DOUBLE, gridx DOUBLE, gridy DOUBLE, skewx DOUBLE, skewy DOUBLE, algorithm VARCHAR, maxerr DOUBLE)
RASTER ST_Resample (rast RASTER, width INTEGER, height INTEGER)
RASTER ST_Resample (rast RASTER, width INTEGER, height INTEGER, gridx DOUBLE)
RASTER ST_Resample (rast RASTER, width INTEGER, height INTEGER, gridx DOUBLE, gridy DOUBLE)
RASTER ST_Resample (rast RASTER, width INTEGER, height INTEGER, gridx DOUBLE, gridy DOUBLE, skewx DOUBLE)
RASTER ST_Resample (rast RASTER, width INTEGER, height INTEGER, gridx DOUBLE, gridy DOUBLE, skewx DOUBLE, skewy DOUBLE)
RASTER ST_Resample (rast RASTER, width INTEGER, height INTEGER, gridx DOUBLE, gridy DOUBLE, skewx DOUBLE, skewy DOUBLE, algorithm VARCHAR)
RASTER ST_Resample (rast RASTER, width INTEGER, height INTEGER, gridx DOUBLE, gridy DOUBLE, skewx DOUBLE, skewy DOUBLE, algorithm VARCHAR, maxerr DOUBLE)
RASTER ST_Resample (rast RASTER, ref RASTER)
RASTER ST_Resample (rast RASTER, ref RASTER, algorithm VARCHAR)
RASTER ST_Resample (rast RASTER, ref RASTER, algorithm VARCHAR, maxerr DOUBLE)
RASTER ST_Resample (rast RASTER, ref RASTER, algorithm VARCHAR, maxerr DOUBLE, usescale BOOLEAN)
RASTER ST_Resample (rast RASTER, ref RASTER, usescale BOOLEAN)
RASTER ST_Resample (rast RASTER, ref RASTER, usescale BOOLEAN, algorithm VARCHAR)
RASTER ST_Resample (rast RASTER, ref RASTER, usescale BOOLEAN, algorithm VARCHAR, maxerr DOUBLE)
```

#### Description

Resamples a raster onto another pixel grid that covers the same area, and returns the new raster.

The target grid is given by a pixel size (`scalex`, `scaley`, in world units; 0 keeps the current size and the sign is ignored), by a size in pixels (`width`, `height`), or by a reference raster `ref` whose alignment, skew, coordinate system and (unless `usescale` is false) pixel size are used. `gridx`/`gridy` are the world coordinates of any pixel corner of the target grid (default: the upper-left corner of the raster's extent) and `skewx`/`skewy` its skew (default 0). The result is north-up (negative `scaley`) unless a reference raster or a skew says otherwise.

`algorithm` is one of `NearestNeighbor` (the default), `Bilinear`, `Cubic`, `CubicSpline`, `Lanczos`, `Average`, `Mode`, `Max`, `Min`. `maxerr` is the error, in pixels, tolerated by the approximation of the coordinate transformation (default 0.125). Pixels outside of the source, and NODATA pixels, are NODATA in the result when the band has a NODATA value, and 0 otherwise.

#### Example

```sql
SELECT ST_Width(r), ST_ScaleX(r) FROM (SELECT ST_Resample(ST_AddBand(ST_MakeEmptyRaster(10, 10, 0, 0, 1), '8BUI', 1), 2.0, 2.0) AS r);
----
5    2.0
```

----

### ST_Rescale


#### Signatures

```sql
RASTER ST_Rescale (rast RASTER, scalexy DOUBLE)
RASTER ST_Rescale (rast RASTER, scalexy DOUBLE, algorithm VARCHAR)
RASTER ST_Rescale (rast RASTER, scalexy DOUBLE, algorithm VARCHAR, maxerr DOUBLE)
RASTER ST_Rescale (rast RASTER, scalex DOUBLE, scaley DOUBLE)
RASTER ST_Rescale (rast RASTER, scalex DOUBLE, scaley DOUBLE, algorithm VARCHAR)
RASTER ST_Rescale (rast RASTER, scalex DOUBLE, scaley DOUBLE, algorithm VARCHAR, maxerr DOUBLE)
```

#### Description

Resamples a raster to a new pixel size, in world units, keeping its extent and upper-left corner. The sign of the scale is ignored: the result is north-up.

See ST_Resample for `algorithm` and `maxerr`. Use ST_SetScale to change the georeference without resampling.

#### Example

```sql
SELECT ST_Width(ST_Rescale(ST_AddBand(ST_MakeEmptyRaster(10, 10, 0, 0, 1), '8BUI', 1), 0.5, 'Bilinear'));
----
20
```

----

### ST_Resize


#### Signatures

```sql
RASTER ST_Resize (rast RASTER, width INTEGER, height INTEGER)
RASTER ST_Resize (rast RASTER, width INTEGER, height INTEGER, algorithm VARCHAR)
RASTER ST_Resize (rast RASTER, width INTEGER, height INTEGER, algorithm VARCHAR, maxerr DOUBLE)
RASTER ST_Resize (rast RASTER, percentwidth DOUBLE, percentheight DOUBLE)
RASTER ST_Resize (rast RASTER, percentwidth DOUBLE, percentheight DOUBLE, algorithm VARCHAR)
RASTER ST_Resize (rast RASTER, percentwidth DOUBLE, percentheight DOUBLE, algorithm VARCHAR, maxerr DOUBLE)
RASTER ST_Resize (rast RASTER, textwidth VARCHAR, textheight VARCHAR)
RASTER ST_Resize (rast RASTER, textwidth VARCHAR, textheight VARCHAR, algorithm VARCHAR)
RASTER ST_Resize (rast RASTER, textwidth VARCHAR, textheight VARCHAR, algorithm VARCHAR, maxerr DOUBLE)
```

#### Description

Resamples a raster to a new width and height, keeping its extent.

The size is given in pixels (integers), as fractions of the current size (doubles: 0.5 halves the size), or as text holding either a number of pixels or a percentage (`'50%'`). The result is north-up. See ST_Resample for `algorithm` and `maxerr`.

#### Example

```sql
SELECT ST_Width(r), ST_Height(r) FROM (SELECT ST_Resize(ST_AddBand(ST_MakeEmptyRaster(10, 10, 0, 0, 1), '8BUI', 1), '50%', '20') AS r);
----
5    20
```

----

### ST_Reskew


#### Signatures

```sql
RASTER ST_Reskew (rast RASTER, skewxy DOUBLE)
RASTER ST_Reskew (rast RASTER, skewxy DOUBLE, algorithm VARCHAR)
RASTER ST_Reskew (rast RASTER, skewxy DOUBLE, algorithm VARCHAR, maxerr DOUBLE)
RASTER ST_Reskew (rast RASTER, skewx DOUBLE, skewy DOUBLE)
RASTER ST_Reskew (rast RASTER, skewx DOUBLE, skewy DOUBLE, algorithm VARCHAR)
RASTER ST_Reskew (rast RASTER, skewx DOUBLE, skewy DOUBLE, algorithm VARCHAR, maxerr DOUBLE)
```

#### Description

Resamples a raster onto a grid with the given skew (rotation terms), keeping the pixel size and covering the same extent.

See ST_Resample for `algorithm` and `maxerr`. Use ST_SetSkew to change the georeference without resampling.

#### Example

```sql
SELECT ST_SkewX(ST_Reskew(ST_AddBand(ST_MakeEmptyRaster(10, 10, 0, 0, 1), '8BUI', 1), 0.1, 0.1));
----
0.1
```

----

### ST_Reverse


#### Signature

```sql
GEOMETRY ST_Reverse (geom GEOMETRY)
```

#### Description

Returns the geometry with the order of its vertices reversed

----

### ST_Rotation


#### Signature

```sql
DOUBLE ST_Rotation (rast RASTER)
```

#### Description

Returns the rotation of the raster in radians, computed from the pixel column direction (`scalex`, `skewy`). A raster that is not rotated returns 0.

#### Example

```sql
SELECT ST_Rotation(ST_MakeEmptyRaster(10, 5, 0, 0, 2, -2, 0, 0));
----
0.0
```

----

### ST_Roughness


#### Signatures

```sql
RASTER ST_Roughness (rast RASTER)
RASTER ST_Roughness (rast RASTER, nband INTEGER)
RASTER ST_Roughness (rast RASTER, nband INTEGER, pixeltype VARCHAR)
RASTER ST_Roughness (rast RASTER, nband INTEGER, pixeltype VARCHAR, interpolate_nodata BOOLEAN)
```

#### Description

Returns the roughness of an elevation band: the difference between the largest and the smallest value in the 3x3 neighbourhood of each pixel.

Computed by GDAL's DEM processing on the 3x3 neighbourhood of each pixel. Pixels on the border and next to NODATA pixels are computed from a neighbourhood that GDAL extrapolates, where PostGIS substitutes the value of the centre pixel, so those pixels can differ from PostGIS. `nband` is 1-based (default 1) and `pixeltype` the pixel type of the result (default `32BF`). NODATA pixels are NODATA in the result (-9999, or the largest value of the pixel type if it cannot hold -9999). `interpolate_nodata` must be false and the `customextent` variants are not available.

#### Example

```sql
SELECT ST_Value(ST_Roughness(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(3, 3, 0, 3, 1), '32BF', 1), 2, 2, 9)), 2, 2);
----
8.0
```

----

### ST_SameAlignment


#### Signature

```sql
BOOLEAN ST_SameAlignment (rast1 RASTER, rast2 RASTER)
```

#### Description

Returns true if two rasters have the same coordinate system, scale and skew and their pixel grids line up (a pixel corner of one falls on a pixel corner of the other). The rasters do not need to overlap.

#### Example

```sql
SELECT ST_SameAlignment(ST_MakeEmptyRaster(2, 2, 0, 0, 1), ST_MakeEmptyRaster(3, 3, 5, -7, 1));
----
true
```

----

### ST_ScaleX


#### Signature

```sql
DOUBLE ST_ScaleX (rast RASTER)
```

#### Description

Returns the X term of the pixel size, in world units per pixel column.

#### Example

```sql
SELECT ST_ScaleX(ST_MakeEmptyRaster(10, 5, 0, 0, 2, -3, 0, 0));
----
2.0
```

----

### ST_ScaleY


#### Signature

```sql
DOUBLE ST_ScaleY (rast RASTER)
```

#### Description

Returns the Y term of the pixel size, in world units per pixel row (negative for north-up rasters).

#### Example

```sql
SELECT ST_ScaleY(ST_MakeEmptyRaster(10, 5, 0, 0, 2, -3, 0, 0));
----
-3.0
```

----

### ST_Scroll


#### Signature

```sql
GEOMETRY ST_Scroll (line GEOMETRY, point GEOMETRY)
```

#### Description

Rotates a closed linestring's start point to the vertex nearest to the given point

#### Example

```sql
SELECT ST_AsText(ST_Scroll(ST_GeomFromText('LINESTRING(0 0, 1 0, 1 1, 0 1, 0 0)'), ST_Point(1, 1)))
```

----

### ST_Segmentize


#### Signatures

```sql
GEOMETRY ST_Segmentize (geom GEOMETRY, max_segment_length DOUBLE)
GEOG ST_Segmentize (geog GEOG, max_segment_length DOUBLE)
```

#### Description

Densifies a geometry by adding vertices so no segment exceeds max_segment_length

----

### ST_SetBandNoDataValue


#### Signatures

```sql
RASTER ST_SetBandNoDataValue (rast RASTER, nodatavalue DOUBLE)
RASTER ST_SetBandNoDataValue (rast RASTER, band INTEGER, nodatavalue DOUBLE)
RASTER ST_SetBandNoDataValue (rast RASTER, band INTEGER, nodatavalue DOUBLE, forcechecking BOOLEAN)
```

#### Description

Returns the raster with the NODATA value of a band (1-based, default 1) set to `nodatavalue`. NULL removes the NODATA value. The pixel values do not change. `forcechecking` is accepted for compatibility and ignored.

#### Example

```sql
SELECT ST_BandNoDataValue(ST_SetBandNoDataValue(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '8BUI'), 255));
----
255.0
```

----

### ST_SetGeoReference


#### Signatures

```sql
RASTER ST_SetGeoReference (rast RASTER, georef VARCHAR)
RASTER ST_SetGeoReference (rast RASTER, georef VARCHAR, format VARCHAR)
RASTER ST_SetGeoReference (rast RASTER, upperleftx DOUBLE, upperlefty DOUBLE, scalex DOUBLE, scaley DOUBLE, skewx DOUBLE, skewy DOUBLE)
```

#### Description

Returns the raster with a new georeference. The pixels are not resampled.

`georef` holds the six world-file terms `scalex skewy skewx scaley upperleftx upperlefty` separated by whitespace. `format` is `GDAL` (the default, the upper-left term is the corner of the upper-left pixel) or `ESRI` (it is the centre of that pixel).

#### Example

```sql
SELECT ST_UpperLeftX(ST_SetGeoReference(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '2 0 0 -2 100 200'));
----
100.0
```

----

### ST_SetPoint


#### Signature

```sql
GEOMETRY ST_SetPoint (line GEOMETRY, position INTEGER, point GEOMETRY)
```

#### Description

Replaces a point in a linestring (0-indexed, negative from end)

#### Example

```sql
SELECT ST_AsText(ST_SetPoint(ST_GeomFromText('LINESTRING(0 0, 1 1, 2 2)'), 1, ST_Point(5, 5)))
```

----

### ST_SetScale


#### Signatures

```sql
RASTER ST_SetScale (rast RASTER, scale DOUBLE)
RASTER ST_SetScale (rast RASTER, scalex DOUBLE, scaley DOUBLE)
```

#### Description

Returns the raster with a new pixel size, in world units. The pixels are not resampled (see ST_Rescale). The single-value variant sets both `scalex` and `scaley` to the same value.

#### Example

```sql
SELECT ST_ScaleY(ST_SetScale(ST_MakeEmptyRaster(2, 2, 0, 0, 1), 2, -3));
----
-3.0
```

----

### ST_SetSkew


#### Signatures

```sql
RASTER ST_SetSkew (rast RASTER, skew DOUBLE)
RASTER ST_SetSkew (rast RASTER, skewx DOUBLE, skewy DOUBLE)
```

#### Description

Returns the raster with new skew terms. The pixels are not resampled (see ST_Reskew). The single-value variant sets both `skewx` and `skewy` to the same value.

#### Example

```sql
SELECT ST_SkewX(ST_SetSkew(ST_MakeEmptyRaster(2, 2, 0, 0, 1), 0.5, 0.25));
----
0.5
```

----

### ST_SetSRID


#### Signatures

```sql
GEOMETRY ST_SetSRID (geom GEOMETRY, srid INTEGER)
RASTER ST_SetSRID (rast RASTER, srid INTEGER)
```

#### Description

Sets the SRID of a geometry (no-op in DuckDB — use GEOMETRY('EPSG:XXXX') type for CRS)

#### Example

```sql
SELECT ST_SetSRID(ST_Point(1, 2), 4326)
```

----

### ST_SetUpperLeft


#### Signature

```sql
RASTER ST_SetUpperLeft (rast RASTER, upperleftx DOUBLE, upperlefty DOUBLE)
```

#### Description

Returns the raster moved so that its upper-left corner is at the given world coordinates.

#### Example

```sql
SELECT ST_UpperLeftY(ST_SetUpperLeft(ST_MakeEmptyRaster(2, 2, 0, 0, 1), 100, 200));
----
200.0
```

----

### ST_SetValue


#### Signatures

```sql
RASTER ST_SetValue (rast RASTER, x INTEGER, y INTEGER, newvalue DOUBLE)
RASTER ST_SetValue (rast RASTER, band INTEGER, x INTEGER, y INTEGER, newvalue DOUBLE)
RASTER ST_SetValue (rast RASTER, geom GEOMETRY, newvalue DOUBLE)
RASTER ST_SetValue (rast RASTER, band INTEGER, geom GEOMETRY, newvalue DOUBLE)
```

#### Description

Returns the raster with one pixel (1-based column `x` and row `y`), or every pixel covered by a geometry, set to `newvalue` in a band (1-based, default 1).

A NULL `newvalue` sets the pixels to the band's NODATA value. The value is clamped to the range of the pixel type. For polygons the pixels whose centre is inside are set, for lines and points the pixels they pass through. A pixel outside of the raster is an error.

#### Example

```sql
SELECT ST_DumpValues(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '8BUI'), 1, 2, 5));
----
[[0.0, 0.0], [5.0, 0.0]]
```

----

### ST_SharedPaths


#### Signature

```sql
GEOMETRY ST_SharedPaths (geom1 GEOMETRY, geom2 GEOMETRY)
```

#### Description

Returns shared paths between two linear geometries

----

### ST_ShiftLongitude


#### Signature

```sql
GEOMETRY ST_ShiftLongitude (geom GEOMETRY)
```

#### Description

Shifts longitude: negative values get +360, values >180 get -360

#### Example

```sql
SELECT ST_AsText(ST_ShiftLongitude(ST_Point(-120, 45)))
```

----

### ST_ShortestLine


#### Signature

```sql
GEOMETRY ST_ShortestLine (geom1 GEOMETRY, geom2 GEOMETRY)
```

#### Description

Returns the shortest line between two geometries

----

### ST_Simplify


#### Signature

```sql
GEOMETRY ST_Simplify (geom GEOMETRY, tolerance DOUBLE)
```

#### Description

Returns a simplified version of the geometry

----

### ST_SimplifyPolygonHull


#### Signature

```sql
GEOMETRY ST_SimplifyPolygonHull (geom GEOMETRY, vertex_fraction DOUBLE)
```

#### Description

Simplifies a polygon while preserving topology

----

### ST_SimplifyPreserveTopology


#### Signature

```sql
GEOMETRY ST_SimplifyPreserveTopology (geom GEOMETRY, tolerance DOUBLE)
```

#### Description

Returns a simplified version of the geometry that preserves topology

----

### ST_SimplifyVW


#### Signature

```sql
GEOMETRY ST_SimplifyVW (geom GEOMETRY, area_threshold DOUBLE)
```

#### Description

Simplifies geometry using the Visvalingam-Whyatt area-based algorithm

#### Example

```sql
SELECT ST_AsText(ST_SimplifyVW(ST_GeomFromText('LINESTRING(0 0, 1 1, 2 0, 3 1, 4 0)'), 0.5))
```

----

### ST_SkewX


#### Signature

```sql
DOUBLE ST_SkewX (rast RASTER)
```

#### Description

Returns the X skew of the georeference: the world X offset per pixel row.

#### Example

```sql
SELECT ST_SkewX(ST_MakeEmptyRaster(10, 5, 0, 0, 2, -3, 0.5, 0.25));
----
0.5
```

----

### ST_SkewY


#### Signature

```sql
DOUBLE ST_SkewY (rast RASTER)
```

#### Description

Returns the Y skew of the georeference: the world Y offset per pixel column.

#### Example

```sql
SELECT ST_SkewY(ST_MakeEmptyRaster(10, 5, 0, 0, 2, -3, 0.5, 0.25));
----
0.25
```

----

### ST_Slope


#### Signatures

```sql
RASTER ST_Slope (rast RASTER)
RASTER ST_Slope (rast RASTER, nband INTEGER)
RASTER ST_Slope (rast RASTER, nband INTEGER, pixeltype VARCHAR)
RASTER ST_Slope (rast RASTER, nband INTEGER, pixeltype VARCHAR, units VARCHAR)
RASTER ST_Slope (rast RASTER, nband INTEGER, pixeltype VARCHAR, units VARCHAR, scale DOUBLE)
RASTER ST_Slope (rast RASTER, nband INTEGER, pixeltype VARCHAR, units VARCHAR, scale DOUBLE, interpolate_nodata BOOLEAN)
```

#### Description

Returns the slope of an elevation band, using Horn's formula.

`units` is `DEGREES` (the default), `RADIANS` or `PERCENT`. `scale` is the ratio of vertical units to horizontal units (default 1; use 111120 for elevations in metres on a longitude/latitude grid).

Computed by GDAL's DEM processing on the 3x3 neighbourhood of each pixel. Pixels on the border and next to NODATA pixels are computed from a neighbourhood that GDAL extrapolates, where PostGIS substitutes the value of the centre pixel, so those pixels can differ from PostGIS. `nband` is 1-based (default 1) and `pixeltype` the pixel type of the result (default `32BF`). NODATA pixels are NODATA in the result (-9999, or the largest value of the pixel type if it cannot hold -9999). `interpolate_nodata` must be false and the `customextent` variants are not available.

#### Example

```sql
SELECT round(ST_Value(ST_Slope(ST_SetValue(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(3, 3, 0, 3, 1), '32BF'), ST_MakeEnvelope(1, 0, 2, 3), 1), ST_MakeEnvelope(2, 0, 3, 3), 2), 1, '32BF', 'DEGREES'), 2, 2), 3);
----
45.0
```

----

### ST_Snap


#### Signature

```sql
GEOMETRY ST_Snap (geom GEOMETRY, snap_to GEOMETRY, tolerance DOUBLE)
```

#### Description

Snaps the vertices and segments of a geometry to another geometry's vertices within the given tolerance

----

### ST_SnapToGrid


#### Signatures

```sql
GEOMETRY ST_SnapToGrid (geom GEOMETRY, size DOUBLE)
RASTER ST_SnapToGrid (rast RASTER, gridx DOUBLE, gridy DOUBLE)
RASTER ST_SnapToGrid (rast RASTER, gridx DOUBLE, gridy DOUBLE, algorithm VARCHAR)
RASTER ST_SnapToGrid (rast RASTER, gridx DOUBLE, gridy DOUBLE, algorithm VARCHAR, maxerr DOUBLE)
RASTER ST_SnapToGrid (rast RASTER, gridx DOUBLE, gridy DOUBLE, algorithm VARCHAR, maxerr DOUBLE, scalex DOUBLE)
RASTER ST_SnapToGrid (rast RASTER, gridx DOUBLE, gridy DOUBLE, algorithm VARCHAR, maxerr DOUBLE, scalex DOUBLE, scaley DOUBLE)
RASTER ST_SnapToGrid (rast RASTER, gridx DOUBLE, gridy DOUBLE, scalex DOUBLE, scaley DOUBLE)
RASTER ST_SnapToGrid (rast RASTER, gridx DOUBLE, gridy DOUBLE, scalex DOUBLE, scaley DOUBLE, algorithm VARCHAR)
RASTER ST_SnapToGrid (rast RASTER, gridx DOUBLE, gridy DOUBLE, scalex DOUBLE, scaley DOUBLE, algorithm VARCHAR, maxerr DOUBLE)
RASTER ST_SnapToGrid (rast RASTER, gridx DOUBLE, gridy DOUBLE, scalexy DOUBLE)
RASTER ST_SnapToGrid (rast RASTER, gridx DOUBLE, gridy DOUBLE, scalexy DOUBLE, algorithm VARCHAR)
RASTER ST_SnapToGrid (rast RASTER, gridx DOUBLE, gridy DOUBLE, scalexy DOUBLE, algorithm VARCHAR, maxerr DOUBLE)
```

#### Description

Snaps all coordinates to a grid of the given size

----

### ST_Split


#### Signature

```sql
GEOMETRY ST_Split (geom GEOMETRY, blade GEOMETRY)
```

#### Description

Splits a geometry by another geometry, returning a geometry collection of the pieces

#### Example

```sql
SELECT ST_AsText(ST_Split(ST_GeomFromText('LINESTRING(0 0, 10 0)'), ST_Point(5, 0)))
```

----

### ST_SRID


#### Signatures

```sql
INTEGER ST_SRID (geom GEOMETRY)
INTEGER ST_SRID (rast RASTER)
```

#### Description

Returns the SRID of a geometry (always 0 — DuckDB uses CRS type metadata instead of per-geometry SRIDs)

#### Example

```sql
SELECT ST_SRID(ST_Point(1, 2))
```

----

### ST_StartPoint


#### Signatures

```sql
GEOMETRY ST_StartPoint (geom GEOMETRY)
POINT_2D ST_StartPoint (line LINESTRING_2D)
```

#### Description

Returns the start point of a LINESTRING.

----

### ST_Subdivide


#### Signature

```sql
GEOMETRY ST_Subdivide (geom GEOMETRY, max_vertices UINTEGER)
```

#### Description

Recursively splits a geometry into sub-geometries until the number of vertices of each are below the threshold given by max_vertices. Accepts any type of input except for a GeometryCollection.Degenerate inputs can lead to results having more than max_vertices vertices due to a recursion depth limit.

----

### ST_Summary


#### Signature

```sql
VARCHAR ST_Summary (geom GEOMETRY)
```

#### Description

Returns a text summary of a geometry

#### Example

```sql
SELECT ST_Summary(ST_Point(1, 2))
```

----

### ST_SummaryStats


#### Signatures

```sql
STRUCT(count BIGINT, sum DOUBLE, mean DOUBLE, stddev DOUBLE, min DOUBLE, max DOUBLE) ST_SummaryStats (rast RASTER)
STRUCT(count BIGINT, sum DOUBLE, mean DOUBLE, stddev DOUBLE, min DOUBLE, max DOUBLE) ST_SummaryStats (rast RASTER, nband INTEGER)
STRUCT(count BIGINT, sum DOUBLE, mean DOUBLE, stddev DOUBLE, min DOUBLE, max DOUBLE) ST_SummaryStats (rast RASTER, nband INTEGER, exclude_nodata_value BOOLEAN)
STRUCT(count BIGINT, sum DOUBLE, mean DOUBLE, stddev DOUBLE, min DOUBLE, max DOUBLE) ST_SummaryStats (rast RASTER, exclude_nodata_value BOOLEAN)
```

#### Description

Returns the count, sum, mean, standard deviation, minimum and maximum of the pixel values of a band, as a struct `(count, sum, mean, stddev, min, max)`.

`nband` is 1-based and defaults to 1. NODATA pixels are left out unless `exclude_nodata_value` is false. `stddev` is the population standard deviation. When no pixel is counted, `count` is 0 and the other fields are NULL.

#### Example

```sql
SELECT ST_SummaryStats(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(2, 1, 0, 0, 1), '8BUI', 1), 1, 1, 5));
----
{'count': 2, 'sum': 6.0, 'mean': 3.0, 'stddev': 2.0, 'min': 1.0, 'max': 5.0}
```

----

### ST_SwapOrdinates


#### Signature

```sql
GEOMETRY ST_SwapOrdinates (geom GEOMETRY, ords VARCHAR)
```

#### Description

Swaps two ordinate values in a geometry (e.g., 'xy' swaps x and y)

#### Example

```sql
SELECT ST_AsText(ST_SwapOrdinates(ST_Point(1, 2), 'xy'))
```

----

### ST_SymDifference


#### Signature

```sql
GEOMETRY ST_SymDifference (geom1 GEOMETRY, geom2 GEOMETRY)
```

#### Description

Returns the symmetric difference of two geometries

----

### ST_Tile


#### Signatures

```sql
RASTER[] ST_Tile (rast RASTER, width INTEGER, height INTEGER)
RASTER[] ST_Tile (rast RASTER, width INTEGER, height INTEGER, padwithnodata BOOLEAN)
RASTER[] ST_Tile (rast RASTER, width INTEGER, height INTEGER, padwithnodata BOOLEAN, nodataval DOUBLE)
RASTER[] ST_Tile (rast RASTER, nband INTEGER, width INTEGER, height INTEGER)
RASTER[] ST_Tile (rast RASTER, nband INTEGER, width INTEGER, height INTEGER, padwithnodata BOOLEAN)
RASTER[] ST_Tile (rast RASTER, nband INTEGER, width INTEGER, height INTEGER, padwithnodata BOOLEAN, nodataval DOUBLE)
RASTER[] ST_Tile (rast RASTER, nbands INTEGER[], width INTEGER, height INTEGER)
RASTER[] ST_Tile (rast RASTER, nbands INTEGER[], width INTEGER, height INTEGER, padwithnodata BOOLEAN)
RASTER[] ST_Tile (rast RASTER, nbands INTEGER[], width INTEGER, height INTEGER, padwithnodata BOOLEAN, nodataval DOUBLE)
```

#### Description

Splits a raster into tiles of `width` x `height` pixels and returns them as a list, row by row from the upper-left corner. PostGIS returns a set of rows: use `UNNEST` to get the same shape.

With `nband` or `nbands` (1-based) the tiles only have those bands. Tiles on the right and bottom edges are smaller unless `padwithnodata` is true, in which case they are padded with `nodataval` (default: the band's NODATA value, or the smallest value of the pixel type).

#### Example

```sql
SELECT len(ST_Tile(ST_AddBand(ST_MakeEmptyRaster(10, 10, 0, 0, 1), '8BUI'), 4, 4));
----
9
```

----

### ST_TileEnvelope


#### Signature

```sql
GEOMETRY ST_TileEnvelope (tile_zoom INTEGER, tile_x INTEGER, tile_y INTEGER)
```

#### Description

The `ST_TileEnvelope` scalar function generates tile envelope rectangular polygons from specified zoom level and tile indices.

This is used in MVT generation to select the features corresponding to the tile extent. The envelope is in the Web Mercator
coordinate reference system (EPSG:3857). The tile pyramid starts at zoom level 0, corresponding to a single tile for the
world. Each zoom level doubles the number of tiles in each direction, such that zoom level 1 is 2 tiles wide by 2 tiles high,
zoom level 2 is 4 tiles wide by 4 tiles high, and so on. Tile indices start at `[x=0, y=0]` at the top left, and increase
down and right. For example, at zoom level 2, the top right tile is `[x=3, y=0]`, the bottom left tile is `[x=0, y=3]`, and
the bottom right is `[x=3, y=3]`.

```sql
SELECT ST_TileEnvelope(2, 3, 1);
```

#### Example

```sql
SELECT ST_TileEnvelope(2, 3, 1);
┌───────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                         st_tileenvelope(2, 3, 1)                                          │
│                                                 geometry                                                  │
├───────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ POLYGON ((1.00188E+07 0, 1.00188E+07 1.00188E+07, 2.00375E+07 1.00188E+07, 2.00375E+07 0, 1.00188E+07 0)) │
└───────────────────────────────────────────────────────────────────────────────────────────────────────────┘
```

----

### ST_Touches


#### Signatures

```sql
BOOLEAN ST_Touches (geom1 GEOMETRY, geom2 GEOMETRY)
BOOLEAN ST_Touches (rast1 RASTER, rast2 RASTER)
BOOLEAN ST_Touches (rast1 RASTER, nband1 INTEGER, rast2 RASTER, nband2 INTEGER)
```

#### Description

Returns true if the geometries touch

----

### ST_TPI


#### Signatures

```sql
RASTER ST_TPI (rast RASTER)
RASTER ST_TPI (rast RASTER, nband INTEGER)
RASTER ST_TPI (rast RASTER, nband INTEGER, pixeltype VARCHAR)
RASTER ST_TPI (rast RASTER, nband INTEGER, pixeltype VARCHAR, interpolate_nodata BOOLEAN)
```

#### Description

Returns the Topographic Position Index of an elevation band: the value of each pixel minus the mean of its eight neighbours.

Computed by GDAL's DEM processing on the 3x3 neighbourhood of each pixel. Pixels on the border and next to NODATA pixels are computed from a neighbourhood that GDAL extrapolates, where PostGIS substitutes the value of the centre pixel, so those pixels can differ from PostGIS. `nband` is 1-based (default 1) and `pixeltype` the pixel type of the result (default `32BF`). NODATA pixels are NODATA in the result (-9999, or the largest value of the pixel type if it cannot hold -9999). `interpolate_nodata` must be false and the `customextent` variants are not available.

#### Example

```sql
SELECT ST_Value(ST_TPI(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(3, 3, 0, 3, 1), '32BF', 1), 2, 2, 9)), 2, 2);
----
8.0
```

----

### ST_Transform


#### Signatures

```sql
BOX_2D ST_Transform (box BOX_2D, source_crs VARCHAR, target_crs VARCHAR)
BOX_2D ST_Transform (box BOX_2D, source_crs VARCHAR, target_crs VARCHAR, always_xy BOOLEAN)
POINT_2D ST_Transform (point POINT_2D, source_crs VARCHAR, target_crs VARCHAR)
POINT_2D ST_Transform (point POINT_2D, source_crs VARCHAR, target_crs VARCHAR, always_xy BOOLEAN)
GEOMETRY ST_Transform (geom GEOMETRY, source_crs VARCHAR, target_crs VARCHAR)
GEOMETRY ST_Transform (geom GEOMETRY, source_crs VARCHAR, target_crs VARCHAR, always_xy BOOLEAN)
GEOMETRY ST_Transform (geom GEOMETRY, target_crs VARCHAR)
GEOMETRY ST_Transform (geom GEOMETRY, target_crs VARCHAR, always_xy BOOLEAN)
RASTER ST_Transform (rast RASTER, crs VARCHAR)
RASTER ST_Transform (rast RASTER, crs VARCHAR, algorithm VARCHAR)
RASTER ST_Transform (rast RASTER, crs VARCHAR, algorithm VARCHAR, maxerr DOUBLE)
RASTER ST_Transform (rast RASTER, crs VARCHAR, algorithm VARCHAR, maxerr DOUBLE, scalex DOUBLE)
RASTER ST_Transform (rast RASTER, crs VARCHAR, algorithm VARCHAR, maxerr DOUBLE, scalex DOUBLE, scaley DOUBLE)
RASTER ST_Transform (rast RASTER, srid INTEGER)
RASTER ST_Transform (rast RASTER, srid INTEGER, algorithm VARCHAR)
RASTER ST_Transform (rast RASTER, srid INTEGER, algorithm VARCHAR, maxerr DOUBLE)
RASTER ST_Transform (rast RASTER, srid INTEGER, algorithm VARCHAR, maxerr DOUBLE, scalex DOUBLE)
RASTER ST_Transform (rast RASTER, srid INTEGER, algorithm VARCHAR, maxerr DOUBLE, scalex DOUBLE, scaley DOUBLE)
RASTER ST_Transform (rast RASTER, srid INTEGER, scalex DOUBLE, scaley DOUBLE)
RASTER ST_Transform (rast RASTER, srid INTEGER, scalex DOUBLE, scaley DOUBLE, algorithm VARCHAR)
RASTER ST_Transform (rast RASTER, srid INTEGER, scalex DOUBLE, scaley DOUBLE, algorithm VARCHAR, maxerr DOUBLE)
RASTER ST_Transform (rast RASTER, srid INTEGER, scalexy DOUBLE)
RASTER ST_Transform (rast RASTER, srid INTEGER, scalexy DOUBLE, algorithm VARCHAR)
RASTER ST_Transform (rast RASTER, srid INTEGER, scalexy DOUBLE, algorithm VARCHAR, maxerr DOUBLE)
RASTER ST_Transform (rast RASTER, alignto RASTER)
RASTER ST_Transform (rast RASTER, alignto RASTER, algorithm VARCHAR)
RASTER ST_Transform (rast RASTER, alignto RASTER, algorithm VARCHAR, maxerr DOUBLE)
```

#### Description

Transforms a geometry between two coordinate systems

The source and target coordinate systems can be specified using any format that the [PROJ library](https://proj.org) supports.

The third optional `always_xy` parameter can be used to force the input and output geometries to be interpreted as having a [easting, northing] coordinate axis order regardless of what the source and target coordinate system definition says. This is particularly useful when transforming to/from the [WGS84/EPSG:4326](https://en.wikipedia.org/wiki/World_Geodetic_System) coordinate system (what most people think of when they hear "longitude"/"latitude" or "GPS coordinates"), which is defined as having a [latitude, longitude] axis order even though [longitude, latitude] is commonly used in practice (e.g. in [GeoJSON](https://tools.ietf.org/html/rfc7946)). More details available in the [PROJ documentation](https://proj.org/en/9.3/faq.html#why-is-the-axis-ordering-in-proj-not-consistent).

DuckDB spatial vendors its own static copy of the PROJ database of coordinate systems, so if you have your own installation of PROJ on your system the available coordinate systems may differ to what's available in other GIS software.

#### Example

```sql
-- Transform a geometry from EPSG:4326 to EPSG:3857 (WGS84 to WebMercator)
-- Note that since WGS84 is defined as having a [latitude, longitude] axis order
-- we follow the standard and provide the input geometry using that axis order,
-- but the output will be [easting, northing] because that is what's defined by
-- WebMercator.

SELECT
    ST_Transform(
        st_point(52.373123, 4.892360),
        'EPSG:4326',
        'EPSG:3857'
    );
----
POINT (544615.0239773799 6867874.103539125)

-- Alternatively, let's say we got our input point from e.g. a GeoJSON file,
-- which uses WGS84 but with [longitude, latitude] axis order. We can use the
-- `always_xy` parameter to force the input geometry to be interpreted as having
-- a [northing, easting] axis order instead, even though the source coordinate
-- reference system definition (WGS84) says otherwise.

SELECT 
    ST_Transform(
        -- note the axis order is reversed here
        st_point(4.892360, 52.373123),
        'EPSG:4326',
        'EPSG:3857',
        always_xy := true
    );
----
POINT (544615.0239773799 6867874.103539125)

-- Transform a geometry from OSG36 British National Grid EPSG:27700 to EPSG:4326 WGS84
-- Standard transform is often fine for the first few decimal places before being wrong
-- which could result in an error starting at about 10m and possibly much more
SELECT ST_Transform(bng, 'EPSG:27700', 'EPSG:4326', xy := true) AS without_grid_file
FROM (SELECT ST_GeomFromText('POINT( 170370.718 11572.405 )') AS bng);
----
POINT (-5.202992651563592 49.96007490162923)

-- By using an official NTv2 grid file, we can reduce the error down around the 9th decimal place
-- which in theory is below a millimetre, and in practise unlikely that your coordinates are that precise
-- British National Grid "NTv2 format files" download available here:
-- https://www.ordnancesurvey.co.uk/products/os-net/for-developers
SELECT ST_Transform(bng
    , '+proj=tmerc +lat_0=49 +lon_0=-2 +k=0.9996012717 +x_0=400000 +y_0=-100000 +ellps=airy +units=m +no_defs +nadgrids=/full/path/to/OSTN15-NTv2/OSTN15_NTv2_OSGBtoETRS.gsb +type=crs'
    , 'EPSG:4326', xy := true) AS with_grid_file
FROM (SELECT ST_GeomFromText('POINT( 170370.718 11572.405 )') AS bng) t;
----
POINT (-5.203046090608746 49.96006137018598)
```

----

### ST_TRI


#### Signatures

```sql
RASTER ST_TRI (rast RASTER)
RASTER ST_TRI (rast RASTER, nband INTEGER)
RASTER ST_TRI (rast RASTER, nband INTEGER, pixeltype VARCHAR)
RASTER ST_TRI (rast RASTER, nband INTEGER, pixeltype VARCHAR, interpolate_nodata BOOLEAN)
```

#### Description

Returns the Terrain Ruggedness Index of an elevation band: the mean absolute difference between each pixel and its eight neighbours.

Computed by GDAL's DEM processing on the 3x3 neighbourhood of each pixel. Pixels on the border and next to NODATA pixels are computed from a neighbourhood that GDAL extrapolates, where PostGIS substitutes the value of the centre pixel, so those pixels can differ from PostGIS. `nband` is 1-based (default 1) and `pixeltype` the pixel type of the result (default `32BF`). NODATA pixels are NODATA in the result (-9999, or the largest value of the pixel type if it cannot hold -9999). `interpolate_nodata` must be false and the `customextent` variants are not available.

#### Example

```sql
SELECT ST_Value(ST_TRI(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(3, 3, 0, 3, 1), '32BF', 1), 2, 2, 9)), 2, 2);
----
8.0
```

----

### ST_TriangulatePolygon


#### Signature

```sql
GEOMETRY ST_TriangulatePolygon (geom GEOMETRY)
```

#### Description

Returns constrained Delaunay triangulation of a polygon

----

### ST_UnaryUnion


#### Signature

```sql
GEOMETRY ST_UnaryUnion (geom GEOMETRY)
```

#### Description

Dissolves a geometry collection into a single geometry

----

### ST_Union


#### Signature

```sql
GEOMETRY ST_Union (geom1 GEOMETRY, geom2 GEOMETRY)
```

#### Description

Returns the union of two geometries

----

### ST_UpperLeftX


#### Signature

```sql
DOUBLE ST_UpperLeftX (rast RASTER)
```

#### Description

Returns the world X coordinate of the upper-left corner of the raster.

#### Example

```sql
SELECT ST_UpperLeftX(ST_MakeEmptyRaster(10, 5, 100, 200, 1));
----
100.0
```

----

### ST_UpperLeftY


#### Signature

```sql
DOUBLE ST_UpperLeftY (rast RASTER)
```

#### Description

Returns the world Y coordinate of the upper-left corner of the raster.

#### Example

```sql
SELECT ST_UpperLeftY(ST_MakeEmptyRaster(10, 5, 100, 200, 1));
----
200.0
```

----

### ST_Value


#### Signatures

```sql
DOUBLE ST_Value (rast RASTER, x INTEGER, y INTEGER)
DOUBLE ST_Value (rast RASTER, x INTEGER, y INTEGER, exclude_nodata_value BOOLEAN)
DOUBLE ST_Value (rast RASTER, band INTEGER, x INTEGER, y INTEGER)
DOUBLE ST_Value (rast RASTER, band INTEGER, x INTEGER, y INTEGER, exclude_nodata_value BOOLEAN)
DOUBLE ST_Value (rast RASTER, pt GEOMETRY)
DOUBLE ST_Value (rast RASTER, pt GEOMETRY, exclude_nodata_value BOOLEAN)
DOUBLE ST_Value (rast RASTER, band INTEGER, pt GEOMETRY)
DOUBLE ST_Value (rast RASTER, band INTEGER, pt GEOMETRY, exclude_nodata_value BOOLEAN)
```

#### Description

Returns the value of a pixel, addressed by its 1-based column `x` and row `y` or by a point in world coordinates.

`band` is 1-based and defaults to 1. NODATA pixels return NULL unless `exclude_nodata_value` is false. A pixel or point outside of the raster returns NULL. The point is not reprojected and only nearest-pixel lookup is supported (PostGIS's `resample` argument is not available).

#### Example

```sql
SELECT ST_Value(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(4, 4, 0, 0, 1), '8BUI'), 2, 3, 42), 2, 3);
----
42.0
```

----

### ST_ValueCount


#### Signatures

```sql
STRUCT("value" DOUBLE, count BIGINT)[] ST_ValueCount (rast RASTER)
STRUCT("value" DOUBLE, count BIGINT)[] ST_ValueCount (rast RASTER, nband INTEGER)
STRUCT("value" DOUBLE, count BIGINT)[] ST_ValueCount (rast RASTER, nband INTEGER, exclude_nodata_value BOOLEAN)
STRUCT("value" DOUBLE, count BIGINT)[] ST_ValueCount (rast RASTER, nband INTEGER, exclude_nodata_value BOOLEAN, searchvalues DOUBLE[])
STRUCT("value" DOUBLE, count BIGINT)[] ST_ValueCount (rast RASTER, nband INTEGER, exclude_nodata_value BOOLEAN, searchvalues DOUBLE[], roundto DOUBLE)
STRUCT("value" DOUBLE, count BIGINT)[] ST_ValueCount (rast RASTER, nband INTEGER, searchvalues DOUBLE[])
STRUCT("value" DOUBLE, count BIGINT)[] ST_ValueCount (rast RASTER, nband INTEGER, searchvalues DOUBLE[], roundto DOUBLE)
STRUCT("value" DOUBLE, count BIGINT)[] ST_ValueCount (rast RASTER, searchvalues DOUBLE[])
STRUCT("value" DOUBLE, count BIGINT)[] ST_ValueCount (rast RASTER, searchvalues DOUBLE[], roundto DOUBLE)
BIGINT ST_ValueCount (rast RASTER, nband INTEGER, exclude_nodata_value BOOLEAN, searchvalue DOUBLE)
BIGINT ST_ValueCount (rast RASTER, nband INTEGER, exclude_nodata_value BOOLEAN, searchvalue DOUBLE, roundto DOUBLE)
BIGINT ST_ValueCount (rast RASTER, nband INTEGER, searchvalue DOUBLE)
BIGINT ST_ValueCount (rast RASTER, nband INTEGER, searchvalue DOUBLE, roundto DOUBLE)
BIGINT ST_ValueCount (rast RASTER, searchvalue DOUBLE)
BIGINT ST_ValueCount (rast RASTER, searchvalue DOUBLE, roundto DOUBLE)
```

#### Description

Counts how often each pixel value occurs in a band.

Without search values the result is a list of structs `(value, count)` for every distinct value, in ascending order; with a list of `searchvalues` it has one entry per search value, in the given order, with a count of 0 for values that do not occur; PostGIS returns a set of rows. With a single `searchvalue` the result is its count. `roundto` rounds the pixel values and the search values to a multiple of it before counting (for example 0.1 or 10; 0, the default, does not round); to round without search values, pass `NULL::DOUBLE[]` as `searchvalues`, because an untyped NULL is taken for a single search value. `nband` is 1-based and defaults to 1; NODATA pixels are left out unless `exclude_nodata_value` is false.

#### Example

```sql
SELECT ST_ValueCount(ST_SetValue(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1), '8BUI', 1), 1, 1, 5));
----
[{'value': 1.0, 'count': 3}, {'value': 5.0, 'count': 1}]
```

----

### ST_VoronoiDiagram


#### Signature

```sql
GEOMETRY ST_VoronoiDiagram (geom GEOMETRY)
```

#### Description

Returns the Voronoi diagram of the supplied MultiPoint geometry

----

### ST_Width


#### Signature

```sql
INTEGER ST_Width (rast RASTER)
```

#### Description

Returns the width of the raster in pixels.

#### Example

```sql
SELECT ST_Width(ST_MakeEmptyRaster(10, 5, 0, 0, 1));
----
10
```

----

### ST_Within


#### Signatures

```sql
BOOLEAN ST_Within (geom1 POINT_2D, geom2 POLYGON_2D)
BOOLEAN ST_Within (geom1 GEOMETRY, geom2 GEOMETRY)
BOOLEAN ST_Within (rast1 RASTER, rast2 RASTER)
BOOLEAN ST_Within (rast1 RASTER, nband1 INTEGER, rast2 RASTER, nband2 INTEGER)
```

#### Description

Returns true if the first geometry is within the second

----

### ST_WithinProperly


#### Signature

```sql
BOOLEAN ST_WithinProperly (geom1 GEOMETRY, geom2 GEOMETRY)
```

#### Description

Returns true if the first geometry \"properly\" is contained by the second geometry

This function functions the same as `ST_ContainsProperly`, but the arguments are swapped.

----

### ST_WorldToRasterCoord


#### Signatures

```sql
STRUCT(columnx INTEGER, rowy INTEGER) ST_WorldToRasterCoord (rast RASTER, xw DOUBLE, yw DOUBLE)
STRUCT(columnx INTEGER, rowy INTEGER) ST_WorldToRasterCoord (rast RASTER, pt GEOMETRY)
```

#### Description

Returns the 1-based column and row of the pixel that contains a world coordinate or a point, as a struct. The result may lie outside of the raster.

#### Example

```sql
SELECT ST_WorldToRasterCoord(ST_MakeEmptyRaster(10, 5, 100, 200, 2, -2, 0, 0), 103, 195);
----
{'columnx': 2, 'rowy': 3}
```

----

### ST_WorldToRasterCoordX


#### Signatures

```sql
INTEGER ST_WorldToRasterCoordX (rast RASTER, xw DOUBLE, yw DOUBLE)
INTEGER ST_WorldToRasterCoordX (rast RASTER, xw DOUBLE)
INTEGER ST_WorldToRasterCoordX (rast RASTER, pt GEOMETRY)
```

#### Description

Returns the 1-based column of the pixel that contains a world coordinate or a point. `yw` may be omitted if the raster is not skewed.

#### Example

```sql
SELECT ST_WorldToRasterCoordX(ST_MakeEmptyRaster(10, 5, 100, 200, 2, -2, 0, 0), 103);
----
2
```

----

### ST_WorldToRasterCoordY


#### Signatures

```sql
INTEGER ST_WorldToRasterCoordY (rast RASTER, xw DOUBLE, yw DOUBLE)
INTEGER ST_WorldToRasterCoordY (rast RASTER, yw DOUBLE)
INTEGER ST_WorldToRasterCoordY (rast RASTER, pt GEOMETRY)
```

#### Description

Returns the 1-based row of the pixel that contains a world coordinate or a point. `xw` may be omitted if the raster is not skewed.

#### Example

```sql
SELECT ST_WorldToRasterCoordY(ST_MakeEmptyRaster(10, 5, 100, 200, 2, -2, 0, 0), 195);
----
3
```

----

### ST_X


#### Signatures

```sql
DOUBLE ST_X (geom GEOMETRY)
DOUBLE ST_X (point POINT_2D)
```

#### Description

Returns the X coordinate of a point geometry

#### Example

```sql
SELECT ST_X(ST_Point(1, 2))
```

----

### ST_XMax


#### Signatures

```sql
DOUBLE ST_XMax (geom GEOMETRY)
DOUBLE ST_XMax (point POINT_2D)
DOUBLE ST_XMax (line LINESTRING_2D)
DOUBLE ST_XMax (polygon POLYGON_2D)
DOUBLE ST_XMax (box BOX_2D)
FLOAT ST_XMax (box BOX_2DF)
```

#### Description

Returns the maximum X coordinate of a geometry

#### Example

```sql
SELECT ST_XMax(ST_Point(1, 2))
```

----

### ST_XMin


#### Signatures

```sql
DOUBLE ST_XMin (geom GEOMETRY)
DOUBLE ST_XMin (point POINT_2D)
DOUBLE ST_XMin (line LINESTRING_2D)
DOUBLE ST_XMin (polygon POLYGON_2D)
DOUBLE ST_XMin (box BOX_2D)
FLOAT ST_XMin (box BOX_2DF)
```

#### Description

Returns the minimum X coordinate of a geometry

#### Example

```sql
SELECT ST_XMin(ST_Point(1, 2))
```

----

### ST_Y


#### Signatures

```sql
DOUBLE ST_Y (geom GEOMETRY)
DOUBLE ST_Y (point POINT_2D)
```

#### Description

Returns the Y coordinate of a point geometry

#### Example

```sql
SELECT ST_Y(ST_Point(1, 2))
```

----

### ST_YMax


#### Signatures

```sql
DOUBLE ST_YMax (geom GEOMETRY)
DOUBLE ST_YMax (point POINT_2D)
DOUBLE ST_YMax (line LINESTRING_2D)
DOUBLE ST_YMax (polygon POLYGON_2D)
DOUBLE ST_YMax (box BOX_2D)
FLOAT ST_YMax (box BOX_2DF)
```

#### Description

Returns the maximum Y coordinate of a geometry

#### Example

```sql
SELECT ST_YMax(ST_Point(1, 2))
```

----

### ST_YMin


#### Signatures

```sql
DOUBLE ST_YMin (geom GEOMETRY)
DOUBLE ST_YMin (point POINT_2D)
DOUBLE ST_YMin (line LINESTRING_2D)
DOUBLE ST_YMin (polygon POLYGON_2D)
DOUBLE ST_YMin (box BOX_2D)
FLOAT ST_YMin (box BOX_2DF)
```

#### Description

Returns the minimum Y coordinate of a geometry

#### Example

```sql
SELECT ST_YMin(ST_Point(1, 2))
```

----

### ST_Z


#### Signature

```sql
DOUBLE ST_Z (geom GEOMETRY)
```

#### Description

Returns the Z coordinate of a point geometry

#### Example

```sql
SELECT ST_Z(ST_Point(1, 2, 3))
```

----

### ST_ZMax


#### Signature

```sql
DOUBLE ST_ZMax (geom GEOMETRY)
```

#### Description

Returns the maximum Z coordinate of a geometry

#### Example

```sql
SELECT ST_ZMax(ST_Point(1, 2, 3))
```

----

### ST_ZMFlag


#### Signature

```sql
UTINYINT ST_ZMFlag (geom GEOMETRY)
```

#### Description

Returns a flag indicating the presence of Z and M values in the input geometry.
0 = No Z or M values
1 = M values only
2 = Z values only
3 = Z and M values

#### Example

```sql
-- ZMFlag for a 2D geometry
SELECT ST_ZMFlag(ST_GeomFromText('POINT(1 1)'));
----
0

-- ZMFlag for a 3DZ geometry
SELECT ST_ZMFlag(ST_GeomFromText('POINT Z(1 1 1)'));
----
2

-- ZMFlag for a 3DM geometry
SELECT ST_ZMFlag(ST_GeomFromText('POINT M(1 1 1)'));
----
1

-- ZMFlag for a 4D geometry
SELECT ST_ZMFlag(ST_GeomFromText('POINT ZM(1 1 1 1)'));
----
3
```

----

### ST_ZMin


#### Signature

```sql
DOUBLE ST_ZMin (geom GEOMETRY)
```

#### Description

Returns the minimum Z coordinate of a geometry

#### Example

```sql
SELECT ST_ZMin(ST_Point(1, 2, 3))
```

----

## Aggregate Functions

### ST_AsMVT


#### Signatures

```sql
BLOB ST_AsMVT (col0 ANY)
BLOB ST_AsMVT (col0 ANY, col1 VARCHAR)
BLOB ST_AsMVT (col0 ANY, col1 VARCHAR, col2 INTEGER)
BLOB ST_AsMVT (col0 ANY, col1 VARCHAR, col2 INTEGER, col3 VARCHAR)
BLOB ST_AsMVT (col0 ANY, col1 VARCHAR, col2 INTEGER, col3 VARCHAR, col4 VARCHAR)
```

#### Description

Make a Mapbox Vector Tile from a set of geometries and properties
The function takes as input a row type (STRUCT) containing a geometry column and any number of property columns.
It returns a single binary BLOB containing the Mapbox Vector Tile.

The function has the following signature:

`ST_AsMVT(row STRUCT, layer_name VARCHAR DEFAULT 'layer', extent INTEGER DEFAULT 4096, geom_column_name VARCHAR DEFAULT NULL, feature_id_column_name VARCHAR DEFAULT NULL) -> BLOB`

- The first argument is a struct containing the geometry and properties.
- The second argument is the name of the layer in the vector tile. This argument is optional and defaults to 'layer'.
- The third argument is the extent of the tile. This argument is optional and defaults to 4096.
- The fourth argument is the name of the geometry column in the input row. This argument is optional. If not provided, the first geometry column in the input row will be used. If multiple geometry columns are present, an error will be raised.
- The fifth argument is the name of the feature id column in the input row. This argument is optional. If provided, the values in this column will be used as feature ids in the vector tile. The column must be of type INTEGER or BIGINT. If set to negative or NULL, a feature id will not be assigned to the corresponding feature.

The input struct must contain exactly one geometry column of type GEOMETRY. It can contain any number of property columns of types VARCHAR, FLOAT, DOUBLE, INTEGER, BIGINT, or BOOLEAN.

Example:
```sql
SELECT ST_AsMVT({'geom': geom, 'id': id, 'name': name}, 'cities', 4096, 'geom', 'id') AS tile
FROM cities;
 ```

This example creates a vector tile named 'cities' with an extent of 4096 from the 'cities' table, using 'geom' as the geometry column and 'id' as the feature id column.

However, you probably want to use the ST_AsMVTGeom function to first transform and clip your geometries to the tile extent.
The following example assumes the geometry is in WebMercator ("EPSG:3857") coordinates.
Replace `{z}`, `{x}`, and `{y}` with the appropriate tile coordinates, `{your table}` with your table name, and `{tile_path}` with the path to write the tile to.

```sql
COPY (
    SELECT ST_AsMVT({{
        "geometry": ST_AsMVTGeom(
            geometry,
            ST_Extent(ST_TileEnvelope({z}, {x}, {y})),
            4096,
            256,
            false
        )
    }})
    FROM {your table} WHERE ST_Intersects(geometry, ST_TileEnvelope({z}, {x}, {y}))
) to {tile_path} (FORMAT 'BLOB');
```

----

### ST_ClusterDBSCAN


#### Signature

```sql
INTEGER ST_ClusterDBSCAN (col0 GEOMETRY, col1 DOUBLE, col2 INTEGER)
```

#### Description

Assigns a DBSCAN cluster ID to each geometry based on spatial proximity.
Returns NULL for noise points. Must be used as a window function.

Parameters:
- geom: input geometry (centroid is used for distance)
- eps: maximum distance between two points to be in the same neighborhood
- minpoints: minimum number of points required to form a dense region

Compatible with PostGIS ST_ClusterDBSCAN.

The whole partition is always clustered: the frame clause and ORDER BY of the window do not affect the result.

#### Example

```sql
SELECT ST_ClusterDBSCAN(geom, 5.0, 3) OVER () as cluster_id
FROM my_points;
```

----

### ST_ClusterIntersecting


#### Signature

```sql
GEOMETRY ST_ClusterIntersecting (col0 GEOMETRY)
```

#### Description

Groups intersecting geometries into clusters (returns geometry collection of collections)

----

### ST_ClusterKMeans


#### Signature

```sql
INTEGER ST_ClusterKMeans (col0 GEOMETRY, col1 INTEGER)
```

#### Description

Assigns a k-means cluster ID to each geometry.
Returns integer cluster IDs (0 to k-1). Must be used as a window function.
Compatible with PostGIS ST_ClusterKMeans.

The whole partition is always clustered: the frame clause and ORDER BY of the window do not affect the result.

#### Example

```sql
SELECT ST_ClusterKMeans(geom, 3) OVER () as cluster_id
FROM my_points;
```

----

### ST_ClusterWithin


#### Signature

```sql
GEOMETRY ST_ClusterWithin (col0 GEOMETRY, col1 DOUBLE)
```

#### Description

Groups geometries within a given distance into clusters

----

### ST_CoverageInvalidEdges_Agg


#### Signatures

```sql
GEOMETRY ST_CoverageInvalidEdges_Agg (col0 GEOMETRY)
GEOMETRY ST_CoverageInvalidEdges_Agg (col0 GEOMETRY, col1 DOUBLE)
```

#### Description

Returns the invalid edges of a coverage geometry

----

### ST_CoverageSimplify_Agg


#### Signatures

```sql
GEOMETRY ST_CoverageSimplify_Agg (col0 GEOMETRY, col1 DOUBLE)
GEOMETRY ST_CoverageSimplify_Agg (col0 GEOMETRY, col1 DOUBLE, col2 BOOLEAN)
```

#### Description

Simplifies a set of geometries while maintaining coverage

----

### ST_CoverageUnion_Agg


#### Signature

```sql
GEOMETRY ST_CoverageUnion_Agg (col0 GEOMETRY)
```

#### Description

Unions a set of geometries while maintaining coverage

----

### ST_Envelope_Agg


#### Signature

```sql
GEOMETRY ST_Envelope_Agg (col0 GEOMETRY)
```

#### Description

Alias for [ST_Extent_Agg](#st_extent_agg).

Computes the minimal-bounding-box polygon containing the set of input geometries.

#### Example

```sql
SELECT ST_Extent_Agg(geom) FROM UNNEST([ST_Point(1,1), ST_Point(5,5)]) AS _(geom);
-- POLYGON ((1 1, 1 5, 5 5, 5 1, 1 1))
```

----

### ST_Extent_Agg


#### Signature

```sql
GEOMETRY ST_Extent_Agg (col0 GEOMETRY)
```

#### Description

Computes the minimal-bounding-box polygon containing the set of input geometries

#### Example

```sql
SELECT ST_Extent_Agg(geom) FROM UNNEST([ST_Point(1,1), ST_Point(5,5)]) AS _(geom);
-- POLYGON ((1 1, 1 5, 5 5, 5 1, 1 1))
```

----

### ST_Intersection_Agg


#### Signature

```sql
GEOMETRY ST_Intersection_Agg (col0 GEOMETRY)
```

#### Description

Computes the intersection of a set of geometries

----

### ST_MemUnion_Agg


#### Signature

```sql
GEOMETRY ST_MemUnion_Agg (col0 GEOMETRY)
```

#### Description

Computes the union of a set of input geometries.
                "Slower, but might be more memory efficient than ST_UnionAgg as each geometry is merged into the union individually rather than all at once.

----

### ST_Retile


#### Signatures

```sql
RASTER[] ST_Retile (rast RASTER, ext GEOMETRY, sfx DOUBLE, sfy DOUBLE, tw INTEGER, th INTEGER)
RASTER[] ST_Retile (rast RASTER, ext GEOMETRY, sfx DOUBLE, sfy DOUBLE, tw INTEGER, th INTEGER, algo VARCHAR)
```

#### Description

Aggregate: rebuilds the rasters of a group, a coverage tiled in any way, as a regular set of tiles, and returns the tiles as a list in row order.

The tiles are `tw` x `th` pixels of `sfx` x `sfy` world units and start at the upper-left corner of the bounding box of `ext`, which they cover completely (the last tiles of a row or column may extend beyond it). Each tile is resampled with `algo` (default `NearestNeighbor`, see ST_Resample) from the rasters that overlap it, later rasters over earlier ones; tiles that no raster overlaps are left out, and pixels that no raster covers are NODATA. The rasters must have the same coordinate system and the same bands; `ext` must be in that coordinate system. The arguments other than the raster are taken from the first row of the group.

In PostGIS, ST_Retile is a set-returning function that takes the name of a table and of its raster column. Here it is an aggregate over the rasters themselves: `SELECT UNNEST(ST_Retile(rast, ...)) FROM coverage`. All rasters of a group are kept in memory until the group is complete.

#### Example

```sql
SELECT len(ST_Retile(rast, ST_MakeEnvelope(150000, 169760, 150320, 170000), 20.0, 20.0, 8, 6)) FROM ST_ReadRaster('test/data/raster/dem.tif', 5, 7);
```

----

### ST_SummaryStatsAgg


#### Signatures

```sql
STRUCT(count BIGINT, sum DOUBLE, mean DOUBLE, stddev DOUBLE, min DOUBLE, max DOUBLE) ST_SummaryStatsAgg (rast RASTER)
STRUCT(count BIGINT, sum DOUBLE, mean DOUBLE, stddev DOUBLE, min DOUBLE, max DOUBLE) ST_SummaryStatsAgg (rast RASTER, nband INTEGER)
STRUCT(count BIGINT, sum DOUBLE, mean DOUBLE, stddev DOUBLE, min DOUBLE, max DOUBLE) ST_SummaryStatsAgg (rast RASTER, nband INTEGER, exclude_nodata_value BOOLEAN)
```

#### Description

Aggregate: returns the count, sum, mean, population standard deviation, minimum and maximum of the pixel values of a band over all rasters of a group, as a struct `(count, sum, mean, stddev, min, max)`.

`nband` is 1-based and defaults to 1; NODATA pixels are left out unless `exclude_nodata_value` is false. NULL rasters are skipped. The `sample_percent` argument of PostGIS is not available: all pixels are always read.

#### Example

```sql
SELECT (ST_SummaryStatsAgg(rast)).mean FROM ST_ReadRaster('test/data/raster/dem.tif', 8, 8);
```

----

### ST_Union_Agg


#### Signatures

```sql
GEOMETRY ST_Union_Agg (col0 GEOMETRY)
RASTER ST_Union_Agg (rast RASTER)
RASTER ST_Union_Agg (rast RASTER, uniontype VARCHAR)
RASTER ST_Union_Agg (rast RASTER, nband INTEGER)
RASTER ST_Union_Agg (rast RASTER, nband INTEGER, uniontype VARCHAR)
```

#### Description

Aggregate: merges the rasters of a group into one raster that covers them all.

The rasters must have the same alignment (see ST_SameAlignment and ST_Resample). Where rasters overlap, `uniontype` decides the value: `LAST` (the default), `FIRST`, `MIN`, `MAX`, `COUNT` (the number of rasters with a value, stored as `32BUI`), `SUM`, `MEAN` (stored as `64BF`) or `RANGE` (the difference between the largest and the smallest value). NODATA pixels do not take part; pixels that no raster gives a value are NODATA (the NODATA value of the first raster, or the smallest value of the pixel type). With `nband` (1-based) the result has that band only, otherwise all bands are merged and the rasters must have the same number of bands. NULL rasters are skipped.

PostGIS calls this aggregate `ST_Union`; that name is a scalar geometry function here. `FIRST` and `LAST` depend on the order of the rows: use `ST_Union_Agg(rast ORDER BY ...)` for a defined result. All rasters of a group are kept in memory until the group is complete.

#### Example

```sql
SELECT ST_Width(ST_Union_Agg(rast, 'MEAN')) FROM ST_ReadRaster('test/data/raster/dem.tif', 8, 8);
```

----

### TopoElementArray_Agg


#### Signature

```sql
INTEGER[2][] TopoElementArray_Agg (col0 INTEGER[2])
```

#### Description

Collects TopoElements into a TopoElementArray.

A TopoElement is an `INTEGER[2]` holding `[element_id, element_type]`, where the type is 1 for a node,
2 for an edge and 3 for a face. An `INTEGER[]` list such as `[face_id, 3]` is cast implicitly and must
hold exactly two values. The result is an `INTEGER[2][]` with one entry per input row, in input order
(use `ORDER BY` inside the call to control it). NULL rows are skipped and the result is NULL when no
row is aggregated. An element holding a NULL raises an error.

Unlike PostGIS there are no `TopoElement` and `TopoElementArray` domain types: plain integer arrays are
used and the element type is not range-checked.

#### Example

```sql
SELECT TopoElementArray_Agg([face_id, 3] ORDER BY face_id) FROM (VALUES (1), (2), (3)) t(face_id);
-- [[1, 3], [2, 3], [3, 3]]
```

----

## Macro Functions

### ST_Rotate


#### Signature

```sql
GEOMETRY ST_Rotate (geom GEOMETRY, radians double)
```

#### Description

Alias of ST_RotateZ

----

### ST_RotateX


#### Signature

```sql
GEOMETRY ST_RotateX (geom GEOMETRY, radians double)
```

#### Description

Rotates a geometry around the X axis. This is a shorthand macro for calling ST_Affine.

#### Example

```sql
-- Rotate a 3D point 90 degrees (π/2 radians) around the X-axis
SELECT ST_RotateX(ST_GeomFromText('POINT Z(0 1 0)'), pi()/2);
----
POINT Z (0 0 1)
```

----

### ST_RotateY


#### Signature

```sql
GEOMETRY ST_RotateY (geom GEOMETRY, radians double)
```

#### Description

Rotates a geometry around the Y axis. This is a shorthand macro for calling ST_Affine.

#### Example

```sql
-- Rotate a 3D point 90 degrees (π/2 radians) around the Y-axis
SELECT ST_RotateY(ST_GeomFromText('POINT Z(1 0 0)'), pi()/2);
----
POINT Z (0 0 -1)
```

----

### ST_RotateZ


#### Signature

```sql
GEOMETRY ST_RotateZ (geom GEOMETRY, radians double)
```

#### Description

Rotates a geometry around the Z axis. This is a shorthand macro for calling ST_Affine.

#### Example

```sql
-- Rotate a point 90 degrees (π/2 radians) around the Z-axis
SELECT ST_RotateZ(ST_Point(1, 0), pi()/2);
----
POINT (0 1)
```

----

### ST_Scale


#### Signatures

```sql
GEOMETRY ST_Scale (geom GEOMETRY, xs double, ys double, zs double)
GEOMETRY ST_Scale (geom GEOMETRY, xs double, ys double)
```

----

### ST_Translate


#### Signatures

```sql
GEOMETRY ST_Translate (geom GEOMETRY, dx double, dy double, dz double)
GEOMETRY ST_Translate (geom GEOMETRY, dx double, dy double)
```

----

### ST_TransScale


#### Signature

```sql
GEOMETRY ST_TransScale (geom GEOMETRY, dx double, dy double, xs double, ys double)
```

#### Description

Translates and then scales a geometry in X and Y direction. This is a shorthand macro for calling ST_Affine.

#### Example

```sql
-- Translate by (1, 2) then scale by (2, 3)
SELECT ST_TransScale(ST_Point(1, 1), 1, 2, 2, 3);
----
POINT (4 9)
```

----

## Table Functions

### CreateTopology

#### Signature

```sql
CreateTopology (col0 VARCHAR)
CreateTopology (col0 VARCHAR, col1 INTEGER)
CreateTopology (col0 VARCHAR, col1 INTEGER, col2 DOUBLE)
CreateTopology (col0 VARCHAR, col1 INTEGER, col2 DOUBLE, col3 BOOLEAN)
```

#### Description

Creates a new, empty topology and returns its id.

`CreateTopology(name, srid := 0, precision := 0, hasz := false)` creates a schema called `name` in the current database, holding the PostGIS topology tables `node(node_id, containing_face, geom)`, `edge_data(edge_id, start_node, end_node, next_left_edge, abs_next_left_edge, next_right_edge, abs_next_right_edge, left_face, right_face, geom)` and `face(face_id, mbr)` with the universal face `0`, the view `edge`, and the sequences `node_node_id_seq`, `edge_data_edge_id_seq` and `face_face_id_seq`. The topology is recorded in `topology.topology(id, name, srid, precision, hasz)`, which is created on first use. When `srid` is positive the geometry columns are typed `GEOMETRY('EPSG:<srid>')`.

Errors: the name is not a plain identifier (letters, digits and underscores, not starting with a digit), or a topology or schema with that name already exists; negative `srid` or `precision`; `hasz = true`.

Differences from PostGIS: the function is not schema-qualified (`CreateTopology`, not `topology.CreateTopology`), topologies are two-dimensional only, there are no foreign keys between the topology tables, and the `topology.layer` table and the `relation` table of the TopoGeometry layer are not created.

The function runs in its own transaction, on a separate connection to the same database. The change is committed as soon as the call returns, independently of the transaction of the caller: a later `ROLLBACK` does not undo it. If the call fails nothing is changed. It cannot see uncommitted changes either. Calling it inside an explicit transaction (`BEGIN`) that already holds uncommitted changes to the database therefore raises an error asking to commit or roll back first; the check covers any change to the database, not only changes to the tables of the topology, because DuckDB does not expose which tables a transaction has updated or deleted from. A conflict with a concurrent transaction that modified the same topology rows is reported as an error too. Inside an explicit transaction the caller may not see the change in the topology tables until it starts a new transaction. Concurrent edits of one database are executed one after the other.

Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`; subqueries and column references are not allowed. Geometry arguments take a `GEOMETRY` (or its WKT text); a geometry whose type carries a coordinate system must match the SRID of the topology.

#### Example

```sql
CALL CreateTopology('city', 31370);
SELECT * FROM topology.topology;
```

----

### DropTopology

#### Signature

```sql
DropTopology (col0 VARCHAR)
```

#### Description

Drops a topology: its schema with everything in it, and its row in `topology.topology`. Returns the text `Topology 'name' dropped`.

Errors: `SQL/MM Spatial exception - invalid topology name` if the topology is not registered. Unlike PostGIS, a schema that is not a registered topology is never dropped.

The function runs in its own transaction, on a separate connection to the same database. The change is committed as soon as the call returns, independently of the transaction of the caller: a later `ROLLBACK` does not undo it. If the call fails nothing is changed. It cannot see uncommitted changes either. Calling it inside an explicit transaction (`BEGIN`) that already holds uncommitted changes to the database therefore raises an error asking to commit or roll back first; the check covers any change to the database, not only changes to the tables of the topology, because DuckDB does not expose which tables a transaction has updated or deleted from. A conflict with a concurrent transaction that modified the same topology rows is reported as an error too. Inside an explicit transaction the caller may not see the change in the topology tables until it starts a new transaction. Concurrent edits of one database are executed one after the other.

Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`; subqueries and column references are not allowed. Geometry arguments take a `GEOMETRY` (or its WKT text); a geometry whose type carries a coordinate system must match the SRID of the topology.

#### Example

```sql
CALL CreateTopology('city');
CALL DropTopology('city');
```

----

### GetEdgeByPoint

#### Signature

```sql
GetEdgeByPoint (col0 VARCHAR, col1 ANY, col2 DOUBLE)
```

#### Description

Returns the id of the edge within `tolerance` of a point, or 0 if there is none.

`GetEdgeByPoint(toponame, point, tolerance)`. Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- invalid point`, `Two or more edges found`.

The function reads through a separate connection and sees committed data only. Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`.

#### Example

```sql
CALL CreateTopology('city');
SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText('LINESTRING(0 0, 10 0)'));
SELECT * FROM GetEdgeByPoint('city', ST_Point(5, 0.5), 1);
-- 1
```

----

### GetFaceByPoint

#### Signature

```sql
GetFaceByPoint (col0 VARCHAR, col1 ANY, col2 DOUBLE)
```

#### Description

Returns the id of the face containing a point, or 0 if the point is in the universal face.

`GetFaceByPoint(toponame, point, tolerance)`: a point strictly inside a face yields that face whatever the tolerance. Otherwise the faces bounded by the edges within `tolerance` of the point are considered; a point in the universal face close to the boundary of a single face yields that face.

Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- invalid point`, `Two or more faces found` (in particular for a point lying on an edge shared by two faces).

The function reads through a separate connection and sees committed data only. Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`.

#### Example

```sql
CALL CreateTopology('city');
SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))'));
SELECT * FROM GetFaceByPoint('city', ST_Point(5, 5), 0);
-- 1
```

----

### GetNodeByPoint

#### Signature

```sql
GetNodeByPoint (col0 VARCHAR, col1 ANY, col2 DOUBLE)
```

#### Description

Returns the id of the node within `tolerance` of a point, or 0 if there is none.

`GetNodeByPoint(toponame, point, tolerance)`. Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- invalid point`, `Two or more nodes found`.

The function reads through a separate connection and sees committed data only. Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`.

#### Example

```sql
CALL CreateTopology('city');
SELECT * FROM ST_AddIsoNode('city', NULL, ST_Point(1, 1));
SELECT * FROM GetNodeByPoint('city', ST_Point(1, 1.5), 1);
-- 1
```

----

### GetNodeEdges

#### Signature

```sql
GetNodeEdges (col0 VARCHAR, col1 INTEGER)
```

#### Description

Returns the edges incident to a node, as rows of `(sequence, edge)` ordered clockwise starting from north.

An edge is positive when it starts at the node and negative when it ends there; a closed edge is reported twice. Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`.

The function reads through a separate connection and sees committed data only. Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`.

#### Example

```sql
CALL CreateTopology('city');
SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText('MULTILINESTRING((0 0, 10 0), (0 0, 0 10))'));
SELECT * FROM GetNodeEdges('city', 1);
```

----

### GetRingEdges

#### Signature

```sql
GetRingEdges (col0 VARCHAR, col1 INTEGER)
GetRingEdges (col0 VARCHAR, col1 INTEGER, col2 INTEGER)
```

#### Description

Returns the ordered set of signed edges met by walking along one side of an edge, as rows of `(sequence, edge)`.

`GetRingEdges(toponame, edge, max_edges := NULL)`: a positive `edge` starts the walk on the left side of the edge in its own direction, a negative one on the right side in the opposite direction. The walk follows `next_left_edge` after a positive edge and `next_right_edge` after a negative one until it is back at the start. An unknown edge yields no rows.

Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `Max traversing limit hit: N` when the ring has more than `max_edges` edges.

The function reads through a separate connection and sees committed data only. Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`.

#### Example

```sql
CALL CreateTopology('city');
SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))'));
SELECT * FROM GetRingEdges('city', 1);
```

----

### GetTopologyID

#### Signature

```sql
GetTopologyID (col0 VARCHAR)
```

#### Description

Returns the id of the topology with the given name, or NULL if there is none.

The function reads through a separate connection and sees committed data only. Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`.

#### Example

```sql
CALL CreateTopology('city');
SELECT * FROM GetTopologyID('city');
```

----

### GetTopologyName

#### Signature

```sql
GetTopologyName (col0 INTEGER)
```

#### Description

Returns the name of the topology with the given id, or NULL if there is none.

The function reads through a separate connection and sees committed data only. Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`.

#### Example

```sql
CALL CreateTopology('city');
SET VARIABLE id = (SELECT * FROM GetTopologyID('city'));
SELECT * FROM GetTopologyName(getvariable('id'));
-- city
```

----

### GetTopologySRID

#### Signature

```sql
GetTopologySRID (col0 VARCHAR)
```

#### Description

Returns the SRID the topology with the given name was created with, or NULL if there is no such topology.

The function reads through a separate connection and sees committed data only. Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`.

#### Example

```sql
CALL CreateTopology('city', 31370);
SELECT * FROM GetTopologySRID('city');
```

----

### pgr_analyzeGraph

#### Signature

```sql
pgr_analyzeGraph (col0 TABLE, col1 ANY)
```

#### Description

Summary of the usual problems of a network topology.

`pgr_analyzeGraph(edges, tolerance)`

The edges are a table-valued argument with the columns `id`, `source`, `target` (integers) and `geom` (LINESTRING). `tolerance` is a non-negative constant in the unit of the coordinates.

The result has the columns `metric` (VARCHAR) and `count` (BIGINT), with one row per metric, in this order:

| Metric | Description |
| --- | --- |
| `isolated_segments` | Edges whose two end vertices are not used by any other edge |
| `dead_ends` | Vertices that are used by a single edge end |
| `potential_gaps` | Dead ends that are within the tolerance of an edge that is not connected to them: probably a missing connection |
| `intersections` | Pairs of edges that cross each other away from their end points: probably a missing vertex |
| `ring_geometries` | Edges whose geometry is a closed and simple line |

Edges with a NULL or empty geometry count for the first two metrics only.

Differences with pgRouting: `pgr_analyzeGraph` reported the counts as notices and stored flags in the vertices table. A table function cannot alter tables, so the counts are returned as rows and the vertices are derived from the `source` and `target` columns of the edges. The function was removed from pgRouting 4.0 and is kept here for convenience.

#### Example

```sql
SELECT * FROM pgr_analyzeGraph((SELECT id, source, target, geom FROM edges), 0.001);
```

----

### pgr_aStar

#### Signature

```sql
pgr_aStar (col0 TABLE, col1 ANY, col2 ANY, epsilon DOUBLE, factor DOUBLE, heuristic INTEGER, directed BOOLEAN)
```

#### Description

Shortest path(s) using the A* algorithm.

`pgr_aStar(edges, start vids, end vids, [directed := true, heuristic := 5, factor := 1, epsilon := 1])`

The edges are passed as a table-valued argument, i.e. a parenthesised subquery, and its columns are matched by name:

| Column | Type | Description |
| --- | --- | --- |
| `id` | integer | Identifier of the edge |
| `source` | integer | Identifier of the first end point vertex |
| `target` | integer | Identifier of the second end point vertex |
| `cost` | numeric | Weight of the edge (`source`, `target`). A negative value means the edge does not exist in that direction |
| `reverse_cost` | numeric | Optional. Weight of the edge (`target`, `source`). A negative value, or a missing column, means the edge does not exist in that direction |

When `directed` is false every non-negative `cost` and `reverse_cost` is usable in both directions.
A NULL in any of these columns raises an error.

The edges additionally need the numeric columns `x1`, `y1` (coordinates of the `source` vertex) and `x2`, `y2` (coordinates of the `target` vertex).

Options:

- `heuristic` (INTEGER, default 5): 0: `h(v) = 0`, 1: `abs(max(dx, dy))`, 2: `abs(min(dx, dy))`, 3: `dx * dx + dy * dy`, 4: `sqrt(dx * dx + dy * dy)`, 5: `abs(dx) + abs(dy)`
- `factor` (DOUBLE, default 1): multiplier that brings the heuristic to the unit of the costs, must be positive
- `epsilon` (DOUBLE, default 1): weight of the heuristic, must be greater than or equal to 1. A larger value is faster and less accurate

The path is a shortest path only when the scaled heuristic never overestimates the remaining cost. With many start or end vertices one search is run per pair.

`start vids` and `end vids` are either a single integer or a list of integers, which covers the one-to-one, one-to-many, many-to-one and many-to-many signatures of pgRouting. Duplicates are ignored and vertices that are not part of the graph are skipped.
They have to be constants: a literal, a prepared statement parameter or `getvariable('name')`.

The result has one row per vertex of each path, ordered by `start_vid`, `end_vid` and position in the path:

| Column | Type | Description |
| --- | --- | --- |
| `seq` | INTEGER | Sequential value starting from 1 |
| `path_seq` | INTEGER | Position in the path, starting from 1 |
| `start_vid` | BIGINT | Identifier of the starting vertex of the path |
| `end_vid` | BIGINT | Identifier of the ending vertex of the path |
| `node` | BIGINT | Identifier of the vertex at this position |
| `edge` | BIGINT | Identifier of the edge used to go to the next vertex, -1 for the last vertex |
| `cost` | DOUBLE | Cost to traverse `edge`, 0 for the last vertex |
| `agg_cost` | DOUBLE | Aggregate cost from `start_vid` to `node` |

No rows are returned for a pair whose end vertex cannot be reached, or whose start and end vertex are the same.

Differences with pgRouting: the edges are a table-valued argument instead of an SQL string, so the query runs inside the calling transaction and can read CTEs and temporary tables; the combinations signature is not available; and when several paths have the same cost the one that is returned may differ from pgRouting's choice (it is deterministic and does not depend on the order of the input rows).

#### Example

```sql
SELECT * FROM pgr_aStar((SELECT id, source, target, cost, reverse_cost, x1, y1, x2, y2 FROM edges), 6, 10, heuristic := 4);
```

----

### pgr_bdAstar

#### Signature

```sql
pgr_bdAstar (col0 TABLE, col1 ANY, col2 ANY, epsilon DOUBLE, factor DOUBLE, heuristic INTEGER, directed BOOLEAN)
```

#### Description

Shortest path(s) using a bidirectional A* search.

`pgr_bdAstar(edges, start vids, end vids, [directed := true, heuristic := 5, factor := 1, epsilon := 1])`

The edges are passed as a table-valued argument, i.e. a parenthesised subquery, and its columns are matched by name:

| Column | Type | Description |
| --- | --- | --- |
| `id` | integer | Identifier of the edge |
| `source` | integer | Identifier of the first end point vertex |
| `target` | integer | Identifier of the second end point vertex |
| `cost` | numeric | Weight of the edge (`source`, `target`). A negative value means the edge does not exist in that direction |
| `reverse_cost` | numeric | Optional. Weight of the edge (`target`, `source`). A negative value, or a missing column, means the edge does not exist in that direction |

When `directed` is false every non-negative `cost` and `reverse_cost` is usable in both directions.
A NULL in any of these columns raises an error.

The edges additionally need the numeric columns `x1`, `y1` (coordinates of the `source` vertex) and `x2`, `y2` (coordinates of the `target` vertex).

Options:

- `heuristic` (INTEGER, default 5): 0: `h(v) = 0`, 1: `abs(max(dx, dy))`, 2: `abs(min(dx, dy))`, 3: `dx * dx + dy * dy`, 4: `sqrt(dx * dx + dy * dy)`, 5: `abs(dx) + abs(dy)`
- `factor` (DOUBLE, default 1): multiplier that brings the heuristic to the unit of the costs, must be positive
- `epsilon` (DOUBLE, default 1): weight of the heuristic, must be greater than or equal to 1. A larger value is faster and less accurate

The path is a shortest path only when the scaled heuristic never overestimates the remaining cost. With many start or end vertices one search is run per pair.

`start vids` and `end vids` are either a single integer or a list of integers, which covers the one-to-one, one-to-many, many-to-one and many-to-many signatures of pgRouting. Duplicates are ignored and vertices that are not part of the graph are skipped.
They have to be constants: a literal, a prepared statement parameter or `getvariable('name')`.

The result has one row per vertex of each path, ordered by `start_vid`, `end_vid` and position in the path:

| Column | Type | Description |
| --- | --- | --- |
| `seq` | INTEGER | Sequential value starting from 1 |
| `path_seq` | INTEGER | Position in the path, starting from 1 |
| `start_vid` | BIGINT | Identifier of the starting vertex of the path |
| `end_vid` | BIGINT | Identifier of the ending vertex of the path |
| `node` | BIGINT | Identifier of the vertex at this position |
| `edge` | BIGINT | Identifier of the edge used to go to the next vertex, -1 for the last vertex |
| `cost` | DOUBLE | Cost to traverse `edge`, 0 for the last vertex |
| `agg_cost` | DOUBLE | Aggregate cost from `start_vid` to `node` |

No rows are returned for a pair whose end vertex cannot be reached, or whose start and end vertex are the same.

Differences with pgRouting: the edges are a table-valued argument instead of an SQL string, so the query runs inside the calling transaction and can read CTEs and temporary tables; the combinations signature is not available; and when several paths have the same cost the one that is returned may differ from pgRouting's choice (it is deterministic and does not depend on the order of the input rows).

#### Example

```sql
SELECT * FROM pgr_bdAstar((SELECT id, source, target, cost, reverse_cost, x1, y1, x2, y2 FROM edges), 6, 10);
```

----

### pgr_bdDijkstra

#### Signature

```sql
pgr_bdDijkstra (col0 TABLE, col1 ANY, col2 ANY, directed BOOLEAN)
```

#### Description

Shortest path(s) using a bidirectional Dijkstra search, which grows one search from the start vertex and one from the end vertex. One search is run per pair of start and end vertices.

`pgr_bdDijkstra(edges, start vids, end vids, [directed := true])`

The edges are passed as a table-valued argument, i.e. a parenthesised subquery, and its columns are matched by name:

| Column | Type | Description |
| --- | --- | --- |
| `id` | integer | Identifier of the edge |
| `source` | integer | Identifier of the first end point vertex |
| `target` | integer | Identifier of the second end point vertex |
| `cost` | numeric | Weight of the edge (`source`, `target`). A negative value means the edge does not exist in that direction |
| `reverse_cost` | numeric | Optional. Weight of the edge (`target`, `source`). A negative value, or a missing column, means the edge does not exist in that direction |

When `directed` is false every non-negative `cost` and `reverse_cost` is usable in both directions.
A NULL in any of these columns raises an error.

`start vids` and `end vids` are either a single integer or a list of integers, which covers the one-to-one, one-to-many, many-to-one and many-to-many signatures of pgRouting. Duplicates are ignored and vertices that are not part of the graph are skipped.
They have to be constants: a literal, a prepared statement parameter or `getvariable('name')`.

The result has one row per vertex of each path, ordered by `start_vid`, `end_vid` and position in the path:

| Column | Type | Description |
| --- | --- | --- |
| `seq` | INTEGER | Sequential value starting from 1 |
| `path_seq` | INTEGER | Position in the path, starting from 1 |
| `start_vid` | BIGINT | Identifier of the starting vertex of the path |
| `end_vid` | BIGINT | Identifier of the ending vertex of the path |
| `node` | BIGINT | Identifier of the vertex at this position |
| `edge` | BIGINT | Identifier of the edge used to go to the next vertex, -1 for the last vertex |
| `cost` | DOUBLE | Cost to traverse `edge`, 0 for the last vertex |
| `agg_cost` | DOUBLE | Aggregate cost from `start_vid` to `node` |

No rows are returned for a pair whose end vertex cannot be reached, or whose start and end vertex are the same.

Differences with pgRouting: the edges are a table-valued argument instead of an SQL string, so the query runs inside the calling transaction and can read CTEs and temporary tables; the combinations signature is not available; and when several paths have the same cost the one that is returned may differ from pgRouting's choice (it is deterministic and does not depend on the order of the input rows).

#### Example

```sql
SELECT * FROM pgr_bdDijkstra((SELECT id, source, target, cost, reverse_cost FROM edges), 6, 10);
```

----

### pgr_boykovKolmogorov

#### Signature

```sql
pgr_boykovKolmogorov (col0 TABLE, col1 ANY, col2 ANY)
```

#### Description

Maximum flow from the source(s) to the sink(s), with the flow carried by each edge.

`pgr_boykovKolmogorov(edges, start vids, end vids)`

The edges are a table-valued argument, i.e. a parenthesised subquery, whose columns are matched by name:

| Column | Type | Description |
| --- | --- | --- |
| `id` | integer | Identifier of the edge |
| `source` | integer | Identifier of the first end point vertex |
| `target` | integer | Identifier of the second end point vertex |
| `capacity` | integer | Capacity of the edge (`source`, `target`). A value that is not positive means the edge does not exist in that direction |
| `reverse_capacity` | integer | Optional. Capacity of the edge (`target`, `source`). A value that is not positive, or a missing column, means the edge does not exist in that direction |

`start vids` and `end vids` are a single integer or a list of integers, given as constants. With several sources or sinks the flow goes from any source to any sink. A vertex cannot be on both sides.

The result has one row per edge direction that carries flow, ordered by `start_vid`, `end_vid` and `edge`:

| Column | Type | Description |
| --- | --- | --- |
| `seq` | INTEGER | Sequential value starting from 1 |
| `edge` | BIGINT | Identifier of the edge |
| `start_vid` | BIGINT | Vertex the flow leaves from |
| `end_vid` | BIGINT | Vertex the flow goes to |
| `flow` | BIGINT | Flow through the edge in that direction |
| `residual_capacity` | BIGINT | Capacity left in that direction |

Differences with pgRouting: the edges are a table-valued argument instead of an SQL string and the combinations signature is not available. `pgr_pushRelabel`, `pgr_edmondsKarp` and `pgr_boykovKolmogorov` are the same function here: the maximum flow is computed with Dinic's algorithm whatever the name. The total flow is the same as with pgRouting, but a maximum flow is generally not unique, so the flow of individual edges may differ.

#### Example

```sql
SELECT * FROM pgr_boykovKolmogorov((SELECT id, source, target, capacity, reverse_capacity FROM edges), 11, 12);
```

----

### pgr_connectedComponents

#### Signature

```sql
pgr_connectedComponents (col0 TABLE)
```

#### Description

Connected components of an undirected graph: two vertices are in the same component when a path exists between them, whatever the direction of the edges.

`pgr_connectedComponents(edges)`

The edges are a table-valued argument with the columns `id`, `source`, `target`, `cost` and optionally `reverse_cost`, as for `pgr_dijkstra`. A negative `cost` or `reverse_cost` means the edge does not exist in that direction; its end points are still vertices of the graph.

The result is ordered by `component` and `node`:

| Column | Type | Description |
| --- | --- | --- |
| `seq` | BIGINT | Sequential value starting from 1 |
| `component` | BIGINT | Identifier of the component: the smallest vertex identifier it contains |
| `node` | BIGINT | Identifier of a vertex of the component |

Differences with pgRouting: the edges are a table-valued argument instead of an SQL string.

#### Example

```sql
SELECT * FROM pgr_connectedComponents((SELECT id, source, target, cost, reverse_cost FROM edges));
```

----

### pgr_createTopology

#### Signature

```sql
pgr_createTopology (col0 TABLE, col1 ANY)
```

#### Description

Builds the topology of a network: gives each edge the identifier of its start and end vertex, snapping end points that are within a tolerance of each other to the same vertex.

`pgr_createTopology(edges, tolerance)`

The edges are a table-valued argument with the columns `id` (integer) and `geom` (LINESTRING). `tolerance` is a non-negative constant in the unit of the coordinates; with 0 only identical points are merged.

The edges are processed by increasing `id`, the start point before the end point. A point takes the identifier of the nearest existing vertex within the tolerance, otherwise it becomes a new vertex with the next identifier, starting from 1. The result therefore does not depend on the order of the input rows.

| Column | Type | Description |
| --- | --- | --- |
| `id` | BIGINT | Identifier of the edge |
| `source` | BIGINT | Identifier of the vertex at the start of the edge, NULL when the geometry is NULL or empty |
| `target` | BIGINT | Identifier of the vertex at the end of the edge, NULL when the geometry is NULL or empty |

Differences with pgRouting: `pgr_createTopology` updated the `source` and `target` columns of the edge table and created a vertices table. A table function cannot alter its input, so the assignment is returned as rows instead, to be joined back or stored by the caller; the vertices can be obtained with `pgr_extractVertices`. The function was removed from pgRouting 4.0 and is kept here for convenience.

#### Example

```sql
CREATE TABLE network AS
SELECT e.*, t.source, t.target
FROM edges e JOIN pgr_createTopology((SELECT id, geom FROM edges), 0.001) t USING (id);
```

----

### pgr_dijkstra

#### Signature

```sql
pgr_dijkstra (col0 TABLE, col1 ANY, col2 ANY, directed BOOLEAN)
```

#### Description

Shortest path(s) using Dijkstra's algorithm.

`pgr_dijkstra(edges, start vids, end vids, [directed := true])`

The edges are passed as a table-valued argument, i.e. a parenthesised subquery, and its columns are matched by name:

| Column | Type | Description |
| --- | --- | --- |
| `id` | integer | Identifier of the edge |
| `source` | integer | Identifier of the first end point vertex |
| `target` | integer | Identifier of the second end point vertex |
| `cost` | numeric | Weight of the edge (`source`, `target`). A negative value means the edge does not exist in that direction |
| `reverse_cost` | numeric | Optional. Weight of the edge (`target`, `source`). A negative value, or a missing column, means the edge does not exist in that direction |

When `directed` is false every non-negative `cost` and `reverse_cost` is usable in both directions.
A NULL in any of these columns raises an error.

`start vids` and `end vids` are either a single integer or a list of integers, which covers the one-to-one, one-to-many, many-to-one and many-to-many signatures of pgRouting. Duplicates are ignored and vertices that are not part of the graph are skipped.
They have to be constants: a literal, a prepared statement parameter or `getvariable('name')`.

The result has one row per vertex of each path, ordered by `start_vid`, `end_vid` and position in the path:

| Column | Type | Description |
| --- | --- | --- |
| `seq` | INTEGER | Sequential value starting from 1 |
| `path_seq` | INTEGER | Position in the path, starting from 1 |
| `start_vid` | BIGINT | Identifier of the starting vertex of the path |
| `end_vid` | BIGINT | Identifier of the ending vertex of the path |
| `node` | BIGINT | Identifier of the vertex at this position |
| `edge` | BIGINT | Identifier of the edge used to go to the next vertex, -1 for the last vertex |
| `cost` | DOUBLE | Cost to traverse `edge`, 0 for the last vertex |
| `agg_cost` | DOUBLE | Aggregate cost from `start_vid` to `node` |

No rows are returned for a pair whose end vertex cannot be reached, or whose start and end vertex are the same.

Differences with pgRouting: the edges are a table-valued argument instead of an SQL string, so the query runs inside the calling transaction and can read CTEs and temporary tables; the combinations signature is not available; and when several paths have the same cost the one that is returned may differ from pgRouting's choice (it is deterministic and does not depend on the order of the input rows).

#### Example

```sql
SELECT * FROM pgr_dijkstra((SELECT id, source, target, cost, reverse_cost FROM edges), 6, 10);

-- one to many on an undirected graph
SELECT * FROM pgr_dijkstra((SELECT id, source, target, cost, reverse_cost FROM edges), 6, [10, 17], directed := false);
```

----

### pgr_dijkstraCost

#### Signature

```sql
pgr_dijkstraCost (col0 TABLE, col1 ANY, col2 ANY, directed BOOLEAN)
```

#### Description

Cost of the shortest path(s) using Dijkstra's algorithm, without the paths themselves.

`pgr_dijkstraCost(edges, start vids, end vids, [directed := true])`

The edges are passed as a table-valued argument, i.e. a parenthesised subquery, and its columns are matched by name:

| Column | Type | Description |
| --- | --- | --- |
| `id` | integer | Identifier of the edge |
| `source` | integer | Identifier of the first end point vertex |
| `target` | integer | Identifier of the second end point vertex |
| `cost` | numeric | Weight of the edge (`source`, `target`). A negative value means the edge does not exist in that direction |
| `reverse_cost` | numeric | Optional. Weight of the edge (`target`, `source`). A negative value, or a missing column, means the edge does not exist in that direction |

When `directed` is false every non-negative `cost` and `reverse_cost` is usable in both directions.
A NULL in any of these columns raises an error.

`start vids` and `end vids` are either a single integer or a list of integers, which covers the one-to-one, one-to-many, many-to-one and many-to-many signatures of pgRouting. Duplicates are ignored and vertices that are not part of the graph are skipped.
They have to be constants: a literal, a prepared statement parameter or `getvariable('name')`.

The result has one row per pair, ordered by `start_vid` and `end_vid`:

| Column | Type | Description |
| --- | --- | --- |
| `start_vid` | BIGINT | Identifier of the starting vertex |
| `end_vid` | BIGINT | Identifier of the ending vertex |
| `agg_cost` | DOUBLE | Cost of the shortest path from `start_vid` to `end_vid` |

Pairs without a path and pairs made of the same vertex twice are not returned.

Differences with pgRouting: the edges are a table-valued argument instead of an SQL string, so the query runs inside the calling transaction and can read CTEs and temporary tables; the combinations signature is not available; and when several paths have the same cost the one that is returned may differ from pgRouting's choice (it is deterministic and does not depend on the order of the input rows).

#### Example

```sql
SELECT * FROM pgr_dijkstraCost((SELECT id, source, target, cost, reverse_cost FROM edges), [6, 1], [10, 17]);
```

----

### pgr_dijkstraCostMatrix

#### Signature

```sql
pgr_dijkstraCostMatrix (col0 TABLE, col1 ANY, directed BOOLEAN)
```

#### Description

Cost matrix between a set of vertices using Dijkstra's algorithm. The result can be fed to `pgr_TSP`.

`pgr_dijkstraCostMatrix(edges, vids, [directed := true])`

`vids` is a constant list of vertex identifiers.

The edges are passed as a table-valued argument, i.e. a parenthesised subquery, and its columns are matched by name:

| Column | Type | Description |
| --- | --- | --- |
| `id` | integer | Identifier of the edge |
| `source` | integer | Identifier of the first end point vertex |
| `target` | integer | Identifier of the second end point vertex |
| `cost` | numeric | Weight of the edge (`source`, `target`). A negative value means the edge does not exist in that direction |
| `reverse_cost` | numeric | Optional. Weight of the edge (`target`, `source`). A negative value, or a missing column, means the edge does not exist in that direction |

When `directed` is false every non-negative `cost` and `reverse_cost` is usable in both directions.
A NULL in any of these columns raises an error.

The result has one row per pair, ordered by `start_vid` and `end_vid`:

| Column | Type | Description |
| --- | --- | --- |
| `start_vid` | BIGINT | Identifier of the starting vertex |
| `end_vid` | BIGINT | Identifier of the ending vertex |
| `agg_cost` | DOUBLE | Cost of the shortest path from `start_vid` to `end_vid` |

Pairs without a path and pairs made of the same vertex twice are not returned.

Differences with pgRouting: the edges are a table-valued argument instead of an SQL string, so the query runs inside the calling transaction and can read CTEs and temporary tables; the combinations signature is not available; and when several paths have the same cost the one that is returned may differ from pgRouting's choice (it is deterministic and does not depend on the order of the input rows).

#### Example

```sql
SELECT * FROM pgr_dijkstraCostMatrix((SELECT id, source, target, cost, reverse_cost FROM edges), [5, 6, 10, 15], directed := false);
```

----

### pgr_drivingDistance

#### Signature

```sql
pgr_drivingDistance (col0 TABLE, col1 ANY, col2 ANY, equicost BOOLEAN, directed BOOLEAN)
```

#### Description

Vertices whose shortest path cost from the root vertex is less than or equal to a distance, together with the shortest path tree that reaches them.

`pgr_drivingDistance(edges, root vids, distance, [directed := true, equicost := false])`

`root vids` is a single integer or a list of integers. When `equicost` is true a vertex is only reported for the root it is closest to (the smallest root identifier wins ties).

The edges are passed as a table-valued argument, i.e. a parenthesised subquery, and its columns are matched by name:

| Column | Type | Description |
| --- | --- | --- |
| `id` | integer | Identifier of the edge |
| `source` | integer | Identifier of the first end point vertex |
| `target` | integer | Identifier of the second end point vertex |
| `cost` | numeric | Weight of the edge (`source`, `target`). A negative value means the edge does not exist in that direction |
| `reverse_cost` | numeric | Optional. Weight of the edge (`target`, `source`). A negative value, or a missing column, means the edge does not exist in that direction |

When `directed` is false every non-negative `cost` and `reverse_cost` is usable in both directions.
A NULL in any of these columns raises an error.

The result is ordered by `start_vid`, `depth` and `node`:

| Column | Type | Description |
| --- | --- | --- |
| `seq` | BIGINT | Sequential value starting from 1 |
| `depth` | BIGINT | Number of edges between `start_vid` and `node` in the shortest path tree |
| `start_vid` | BIGINT | Identifier of the root vertex |
| `pred` | BIGINT | Predecessor of `node` in the tree, the root itself for the root |
| `node` | BIGINT | Identifier of the reached vertex |
| `edge` | BIGINT | Identifier of the edge used to arrive to `node`, -1 for the root |
| `cost` | DOUBLE | Cost to traverse `edge` |
| `agg_cost` | DOUBLE | Aggregate cost from `start_vid` to `node` |

Differences with pgRouting: the edges are a table-valued argument instead of an SQL string, so the query runs inside the calling transaction and can read CTEs and temporary tables; the combinations signature is not available; and when several paths have the same cost the one that is returned may differ from pgRouting's choice (it is deterministic and does not depend on the order of the input rows).

#### Example

```sql
SELECT * FROM pgr_drivingDistance((SELECT id, source, target, cost, reverse_cost FROM edges), 11, 3.0);

SELECT * FROM pgr_drivingDistance((SELECT id, source, target, cost, reverse_cost FROM edges), [11, 16], 3.0, equicost := true);
```

----

### pgr_edmondsKarp

#### Signature

```sql
pgr_edmondsKarp (col0 TABLE, col1 ANY, col2 ANY)
```

#### Description

Maximum flow from the source(s) to the sink(s), with the flow carried by each edge.

`pgr_edmondsKarp(edges, start vids, end vids)`

The edges are a table-valued argument, i.e. a parenthesised subquery, whose columns are matched by name:

| Column | Type | Description |
| --- | --- | --- |
| `id` | integer | Identifier of the edge |
| `source` | integer | Identifier of the first end point vertex |
| `target` | integer | Identifier of the second end point vertex |
| `capacity` | integer | Capacity of the edge (`source`, `target`). A value that is not positive means the edge does not exist in that direction |
| `reverse_capacity` | integer | Optional. Capacity of the edge (`target`, `source`). A value that is not positive, or a missing column, means the edge does not exist in that direction |

`start vids` and `end vids` are a single integer or a list of integers, given as constants. With several sources or sinks the flow goes from any source to any sink. A vertex cannot be on both sides.

The result has one row per edge direction that carries flow, ordered by `start_vid`, `end_vid` and `edge`:

| Column | Type | Description |
| --- | --- | --- |
| `seq` | INTEGER | Sequential value starting from 1 |
| `edge` | BIGINT | Identifier of the edge |
| `start_vid` | BIGINT | Vertex the flow leaves from |
| `end_vid` | BIGINT | Vertex the flow goes to |
| `flow` | BIGINT | Flow through the edge in that direction |
| `residual_capacity` | BIGINT | Capacity left in that direction |

Differences with pgRouting: the edges are a table-valued argument instead of an SQL string and the combinations signature is not available. `pgr_pushRelabel`, `pgr_edmondsKarp` and `pgr_boykovKolmogorov` are the same function here: the maximum flow is computed with Dinic's algorithm whatever the name. The total flow is the same as with pgRouting, but a maximum flow is generally not unique, so the flow of individual edges may differ.

#### Example

```sql
SELECT * FROM pgr_edmondsKarp((SELECT id, source, target, capacity, reverse_capacity FROM edges), 11, 12);
```

----

### pgr_extractVertices

#### Signature

```sql
pgr_extractVertices (col0 TABLE)
```

#### Description

Vertices of a graph, extracted from its edges.

`pgr_extractVertices(edges)`

The edges are a table-valued argument, i.e. a parenthesised subquery, whose columns are matched by name. One of the following sets of columns is used, in this order of preference:

- `geom` (LINESTRING): the vertices are the distinct start and end points of the lines. They are numbered from 1 by increasing `x`, then `y`.
- `startpoint` and `endpoint` (POINT): same as above, with the end points given explicitly.
- `source` and `target` (integer): the vertices are the distinct identifiers. `x`, `y` and `geom` are NULL.

`id` (integer) is optional. When it is present `in_edges` and `out_edges` are filled, otherwise they are NULL. Rows whose geometry is NULL or empty are skipped.

| Column | Type | Description |
| --- | --- | --- |
| `id` | BIGINT | Identifier of the vertex |
| `in_edges` | BIGINT[] | Sorted identifiers of the edges that end at the vertex, NULL when there is none |
| `out_edges` | BIGINT[] | Sorted identifiers of the edges that start at the vertex, NULL when there is none |
| `x` | DOUBLE | X coordinate of the vertex |
| `y` | DOUBLE | Y coordinate of the vertex |
| `geom` | GEOMETRY | POINT geometry of the vertex, in the coordinate system of the input |

Differences with pgRouting: the edges are a table-valued argument instead of an SQL string, there is no `dryrun` option, and only the X and Y coordinates are considered.

#### Example

```sql
CREATE TABLE vertices AS SELECT * FROM pgr_extractVertices((SELECT id, geom FROM edges));

-- fill the source and target of the edges
SELECT e.id, s.id AS source, t.id AS target
FROM edges e
JOIN vertices s ON ST_Equals(ST_StartPoint(e.geom), s.geom)
JOIN vertices t ON ST_Equals(ST_EndPoint(e.geom), t.geom);
```

----

### pgr_KSP

#### Signature

```sql
pgr_KSP (col0 TABLE, col1 ANY, col2 ANY, col3 ANY, heap_paths BOOLEAN, directed BOOLEAN)
```

#### Description

K shortest loopless paths using Yen's algorithm.

`pgr_KSP(edges, start vids, end vids, K, [directed := true, heap_paths := false])`

At most `K` paths are returned per pair of start and end vertices, by increasing cost. When `heap_paths` is true the candidate paths that were found while searching are returned as well, after the K shortest ones.

The edges are passed as a table-valued argument, i.e. a parenthesised subquery, and its columns are matched by name:

| Column | Type | Description |
| --- | --- | --- |
| `id` | integer | Identifier of the edge |
| `source` | integer | Identifier of the first end point vertex |
| `target` | integer | Identifier of the second end point vertex |
| `cost` | numeric | Weight of the edge (`source`, `target`). A negative value means the edge does not exist in that direction |
| `reverse_cost` | numeric | Optional. Weight of the edge (`target`, `source`). A negative value, or a missing column, means the edge does not exist in that direction |

When `directed` is false every non-negative `cost` and `reverse_cost` is usable in both directions.
A NULL in any of these columns raises an error.

`start vids` and `end vids` are either a single integer or a list of integers, which covers the one-to-one, one-to-many, many-to-one and many-to-many signatures of pgRouting. Duplicates are ignored and vertices that are not part of the graph are skipped.
They have to be constants: a literal, a prepared statement parameter or `getvariable('name')`.

The result has the columns `seq`, `path_id`, `path_seq`, `start_vid`, `end_vid`, `node`, `edge`, `cost` and `agg_cost`. `path_id` numbers the paths from 1 across the whole result, the other columns are those of `pgr_dijkstra`.

Differences with pgRouting: the edges are a table-valued argument instead of an SQL string, so the query runs inside the calling transaction and can read CTEs and temporary tables; the combinations signature is not available; and when several paths have the same cost the one that is returned may differ from pgRouting's choice (it is deterministic and does not depend on the order of the input rows).

#### Example

```sql
SELECT * FROM pgr_KSP((SELECT id, source, target, cost, reverse_cost FROM edges), 6, 17, 2);
```

----

### pgr_maxFlow

#### Signature

```sql
pgr_maxFlow (col0 TABLE, col1 ANY, col2 ANY)
```

#### Description

Value of the maximum flow from the source(s) to the sink(s), computed with Dinic's algorithm.

`pgr_maxFlow(edges, start vids, end vids)`

The edges are a table-valued argument, i.e. a parenthesised subquery, whose columns are matched by name:

| Column | Type | Description |
| --- | --- | --- |
| `id` | integer | Identifier of the edge |
| `source` | integer | Identifier of the first end point vertex |
| `target` | integer | Identifier of the second end point vertex |
| `capacity` | integer | Capacity of the edge (`source`, `target`). A value that is not positive means the edge does not exist in that direction |
| `reverse_capacity` | integer | Optional. Capacity of the edge (`target`, `source`). A value that is not positive, or a missing column, means the edge does not exist in that direction |

`start vids` and `end vids` are a single integer or a list of integers, given as constants. With several sources or sinks the flow goes from any source to any sink. A vertex cannot be on both sides.

The result is a single row with the column `pgr_maxflow` (BIGINT), which is 0 when no sink can be reached. By the max-flow min-cut theorem it is also the capacity of the minimum cut that separates the sources from the sinks.

Differences with pgRouting: the edges are a table-valued argument instead of an SQL string, the function is a table function that returns one row instead of a scalar, and the combinations signature is not available.

#### Example

```sql
SELECT * FROM pgr_maxFlow((SELECT id, source, target, capacity, reverse_capacity FROM edges), 11, 12);
```

----

### pgr_maxFlowMinCost

#### Signature

```sql
pgr_maxFlowMinCost (col0 TABLE, col1 ANY, col2 ANY)
```

#### Description

Maximum flow of minimum cost from the source(s) to the sink(s): among all the maximum flows, one whose total cost is the smallest.

`pgr_maxFlowMinCost(edges, start vids, end vids)`

The edges are a table-valued argument, i.e. a parenthesised subquery, whose columns are matched by name:

| Column | Type | Description |
| --- | --- | --- |
| `id` | integer | Identifier of the edge |
| `source` | integer | Identifier of the first end point vertex |
| `target` | integer | Identifier of the second end point vertex |
| `capacity` | integer | Capacity of the edge (`source`, `target`). A value that is not positive means the edge does not exist in that direction |
| `reverse_capacity` | integer | Optional. Capacity of the edge (`target`, `source`). A value that is not positive, or a missing column, means the edge does not exist in that direction |

`start vids` and `end vids` are a single integer or a list of integers, given as constants. With several sources or sinks the flow goes from any source to any sink. A vertex cannot be on both sides.

In addition to the columns above the edges need `cost` (numeric): the cost of sending one unit of flow from `source` to `target`, and `reverse_cost` (numeric, required when `reverse_capacity` is used): the cost of one unit from `target` to `source`. The existence of an edge direction is decided by its capacity only, and the cost of a usable direction cannot be negative.

The result has one row per edge direction that carries flow, ordered by `source`, `target` and `edge`:

| Column | Type | Description |
| --- | --- | --- |
| `seq` | INTEGER | Sequential value starting from 1 |
| `edge` | BIGINT | Identifier of the edge |
| `source` | BIGINT | Vertex the flow leaves from |
| `target` | BIGINT | Vertex the flow goes to |
| `flow` | BIGINT | Flow through the edge in that direction |
| `residual_capacity` | BIGINT | Capacity left in that direction |
| `cost` | DOUBLE | Cost of the flow through the edge: `flow` times the unit cost |
| `agg_cost` | DOUBLE | Aggregate cost up to this row, the last row holds the total cost |

The flow is computed with the successive shortest path algorithm, which runs one shortest path search per augmentation.

Differences with pgRouting: the edges are a table-valued argument instead of an SQL string and the combinations signature is not available. `pgr_maxFlowMinCost` is the pgRouting name, `pgr_minCostMaxFlow` is an alias. The total flow and the total cost are the same as with pgRouting, but the optimal flow is generally not unique, so the flow of individual edges may differ.

#### Example

```sql
SELECT * FROM pgr_maxFlowMinCost((SELECT id, source, target, capacity, reverse_capacity, cost, reverse_cost FROM edges), 11, 12);
```

----

### pgr_minCostMaxFlow

#### Signature

```sql
pgr_minCostMaxFlow (col0 TABLE, col1 ANY, col2 ANY)
```

#### Description

Maximum flow of minimum cost from the source(s) to the sink(s): among all the maximum flows, one whose total cost is the smallest.

`pgr_minCostMaxFlow(edges, start vids, end vids)`

The edges are a table-valued argument, i.e. a parenthesised subquery, whose columns are matched by name:

| Column | Type | Description |
| --- | --- | --- |
| `id` | integer | Identifier of the edge |
| `source` | integer | Identifier of the first end point vertex |
| `target` | integer | Identifier of the second end point vertex |
| `capacity` | integer | Capacity of the edge (`source`, `target`). A value that is not positive means the edge does not exist in that direction |
| `reverse_capacity` | integer | Optional. Capacity of the edge (`target`, `source`). A value that is not positive, or a missing column, means the edge does not exist in that direction |

`start vids` and `end vids` are a single integer or a list of integers, given as constants. With several sources or sinks the flow goes from any source to any sink. A vertex cannot be on both sides.

In addition to the columns above the edges need `cost` (numeric): the cost of sending one unit of flow from `source` to `target`, and `reverse_cost` (numeric, required when `reverse_capacity` is used): the cost of one unit from `target` to `source`. The existence of an edge direction is decided by its capacity only, and the cost of a usable direction cannot be negative.

The result has one row per edge direction that carries flow, ordered by `source`, `target` and `edge`:

| Column | Type | Description |
| --- | --- | --- |
| `seq` | INTEGER | Sequential value starting from 1 |
| `edge` | BIGINT | Identifier of the edge |
| `source` | BIGINT | Vertex the flow leaves from |
| `target` | BIGINT | Vertex the flow goes to |
| `flow` | BIGINT | Flow through the edge in that direction |
| `residual_capacity` | BIGINT | Capacity left in that direction |
| `cost` | DOUBLE | Cost of the flow through the edge: `flow` times the unit cost |
| `agg_cost` | DOUBLE | Aggregate cost up to this row, the last row holds the total cost |

The flow is computed with the successive shortest path algorithm, which runs one shortest path search per augmentation.

Differences with pgRouting: the edges are a table-valued argument instead of an SQL string and the combinations signature is not available. `pgr_maxFlowMinCost` is the pgRouting name, `pgr_minCostMaxFlow` is an alias. The total flow and the total cost are the same as with pgRouting, but the optimal flow is generally not unique, so the flow of individual edges may differ.

#### Example

```sql
SELECT * FROM pgr_minCostMaxFlow((SELECT id, source, target, capacity, reverse_capacity, cost, reverse_cost FROM edges), 11, 12);
```

----

### pgr_nodeNetwork

#### Signature

```sql
pgr_nodeNetwork (col0 TABLE, col1 ANY)
```

#### Description

Nodes a network: splits the lines where they meet, so that lines only touch at their end points.

`pgr_nodeNetwork(edges, tolerance)`

The edges are a table-valued argument with the columns `id` (integer) and `geom` (LINESTRING). `tolerance` is a non-negative constant in the unit of the coordinates.

A line is split where another line crosses or touches it, at both ends of a stretch it shares with another line, and where another line ends within the tolerance of it without touching it. No split is made within the tolerance of an end of the line or of the previous split. A line does not split itself. Every line is returned, in one piece when nothing splits it.

| Column | Type | Description |
| --- | --- | --- |
| `id` | BIGINT | Identifier of the new edge, from 1, by `old_id` and `sub_id` |
| `old_id` | BIGINT | Identifier of the original edge |
| `sub_id` | INTEGER | Position of the piece along the original edge, starting from 1 |
| `geom` | GEOMETRY | LINESTRING geometry of the piece, in the coordinate system of the input |

Rows whose geometry is NULL or empty are skipped. Only the X and Y coordinates are kept.

Differences with pgRouting: `pgr_nodeNetwork` created a new table. A table function cannot create tables, so the noded edges are returned as rows. The function was removed from pgRouting 4.0 and is kept here for convenience.

#### Example

```sql
CREATE TABLE edges_noded AS SELECT * FROM pgr_nodeNetwork((SELECT id, geom FROM edges), 0.001);
```

----

### pgr_pushRelabel

#### Signature

```sql
pgr_pushRelabel (col0 TABLE, col1 ANY, col2 ANY)
```

#### Description

Maximum flow from the source(s) to the sink(s), with the flow carried by each edge.

`pgr_pushRelabel(edges, start vids, end vids)`

The edges are a table-valued argument, i.e. a parenthesised subquery, whose columns are matched by name:

| Column | Type | Description |
| --- | --- | --- |
| `id` | integer | Identifier of the edge |
| `source` | integer | Identifier of the first end point vertex |
| `target` | integer | Identifier of the second end point vertex |
| `capacity` | integer | Capacity of the edge (`source`, `target`). A value that is not positive means the edge does not exist in that direction |
| `reverse_capacity` | integer | Optional. Capacity of the edge (`target`, `source`). A value that is not positive, or a missing column, means the edge does not exist in that direction |

`start vids` and `end vids` are a single integer or a list of integers, given as constants. With several sources or sinks the flow goes from any source to any sink. A vertex cannot be on both sides.

The result has one row per edge direction that carries flow, ordered by `start_vid`, `end_vid` and `edge`:

| Column | Type | Description |
| --- | --- | --- |
| `seq` | INTEGER | Sequential value starting from 1 |
| `edge` | BIGINT | Identifier of the edge |
| `start_vid` | BIGINT | Vertex the flow leaves from |
| `end_vid` | BIGINT | Vertex the flow goes to |
| `flow` | BIGINT | Flow through the edge in that direction |
| `residual_capacity` | BIGINT | Capacity left in that direction |

Differences with pgRouting: the edges are a table-valued argument instead of an SQL string and the combinations signature is not available. `pgr_pushRelabel`, `pgr_edmondsKarp` and `pgr_boykovKolmogorov` are the same function here: the maximum flow is computed with Dinic's algorithm whatever the name. The total flow is the same as with pgRouting, but a maximum flow is generally not unique, so the flow of individual edges may differ.

#### Example

```sql
SELECT * FROM pgr_pushRelabel((SELECT id, source, target, capacity, reverse_capacity FROM edges), 11, 12);
```

----

### pgr_strongComponents

#### Signature

```sql
pgr_strongComponents (col0 TABLE)
```

#### Description

Strongly connected components of a directed graph, using Tarjan's algorithm: two vertices are in the same component when each one can be reached from the other.

`pgr_strongComponents(edges)`

The edges are a table-valued argument with the columns `id`, `source`, `target`, `cost` and optionally `reverse_cost`, as for `pgr_dijkstra`. A negative `cost` or `reverse_cost` means the edge does not exist in that direction; its end points are still vertices of the graph.

The result is ordered by `component` and `node`:

| Column | Type | Description |
| --- | --- | --- |
| `seq` | BIGINT | Sequential value starting from 1 |
| `component` | BIGINT | Identifier of the component: the smallest vertex identifier it contains |
| `node` | BIGINT | Identifier of a vertex of the component |

Differences with pgRouting: the edges are a table-valued argument instead of an SQL string.

#### Example

```sql
SELECT * FROM pgr_strongComponents((SELECT id, source, target, cost, reverse_cost FROM edges));
```

----

### pgr_trsp

#### Signature

```sql
pgr_trsp (col0 TABLE, col1 ANY, col2 ANY, col3 ANY, directed BOOLEAN)
```

#### Description

Shortest path(s) with turn restrictions.

`pgr_trsp(edges, restrictions, start vids, end vids, [directed := true])`

The edges are a table-valued argument with the columns `id`, `source`, `target`, `cost` and optionally `reverse_cost`, as for `pgr_dijkstra`.

The restrictions are a constant list of structs with the fields:

| Field | Type | Description |
| --- | --- | --- |
| `path` | list of integers | Sequence of edge identifiers that make up the restricted manoeuvre |
| `cost` | numeric | Cost that is added when the whole sequence is travelled. Use `'infinity'::DOUBLE` to forbid the manoeuvre |

Other fields, such as an identifier, are ignored. A table-valued argument can only be used once per call, so the list has to be built beforehand, for example with `SET VARIABLE restrictions = (SELECT list(r) FROM restrictions r)` and passed as `getvariable('restrictions')`.

`start vids` and `end vids` are a single integer or a list of integers.

The result has the columns `seq`, `path_seq`, `start_vid`, `end_vid`, `node`, `edge`, `cost` and `agg_cost` of `pgr_dijkstra`. The cost of a restriction is included in the `cost` of the edge that completes it. A vertex can appear more than once in a path when a detour is cheaper than a restricted manoeuvre, but as in pgRouting an edge is never followed by a U-turn on that same edge.

Differences with pgRouting: the edges are a table-valued argument and the restrictions a list of structs instead of two SQL strings; the combinations signature is not available; and the search tracks how much of each restriction has been travelled, so that the result is the cheapest path for restrictions of any length, including overlapping ones.

#### Example

```sql
SET VARIABLE restrictions = (SELECT list(r) FROM (SELECT path, cost FROM restrictions) r);

SELECT * FROM pgr_trsp((SELECT id, source, target, cost, reverse_cost FROM edges), getvariable('restrictions'), 6, 10);
```

----

### pgr_TSP

#### Signature

```sql
pgr_TSP (col0 TABLE, end_id BIGINT, start_id BIGINT)
```

#### Description

Travelling salesperson tour over a cost matrix: a round trip that visits every node once.

`pgr_TSP(matrix, [start_id := 0, end_id := 0])`

The matrix is a table-valued argument, typically the result of `pgr_dijkstraCostMatrix` with `directed := false`, whose columns are matched by name:

| Column | Type | Description |
| --- | --- | --- |
| `start_vid` | integer | Identifier of the starting node |
| `end_vid` | integer | Identifier of the ending node |
| `agg_cost` | numeric | Cost to go from `start_vid` to `end_vid` |

The problem is solved on an undirected graph: when the costs of the two directions differ the smallest one is used, rows with a negative cost and rows from a node to itself are ignored, and missing cells are completed with the cost of the shortest path through the other nodes. Nodes that cannot be reached from the start node are left out of the tour.

`start_id` is the node where the tour starts and ends, by default (0) the smallest node identifier. When `end_id` is given and differs from `start_id`, it is the last node visited before returning to the start.

The result is ordered by `seq`:

| Column | Type | Description |
| --- | --- | --- |
| `seq` | INTEGER | Sequential value starting from 1 |
| `node` | BIGINT | Identifier of the node at this position. The start node is repeated in the last row |
| `cost` | DOUBLE | Cost to travel from the previous node to `node`, 0 for the first row |
| `agg_cost` | DOUBLE | Aggregate cost from the start node to `node` |

The tour is optimal up to 12 nodes (exact dynamic programming). Beyond that it is a heuristic: a nearest neighbour tour improved with 2-opt and Or-opt moves until no move shortens it, which gives a good but not necessarily optimal tour.

Differences with pgRouting: the matrix is a table-valued argument instead of an SQL string; pgRouting uses the metric approximation of the Boost graph library, so the tours differ although both are valid (the tour returned here is never longer than the nearest neighbour tour, and does not depend on the order of the input rows); and a start and end node that are not connected raise an error instead of being joined with an estimated cost.

#### Example

```sql
SELECT * FROM pgr_TSP((
    SELECT * FROM pgr_dijkstraCostMatrix((SELECT id, source, target, cost, reverse_cost FROM edges), [1, 5, 9, 15], directed := false)
), start_id := 1);
```

----

### pgr_withPoints

#### Signature

```sql
pgr_withPoints (col0 TABLE, col1 ANY, col2 ANY, col3 ANY, details BOOLEAN, directed BOOLEAN, driving_side VARCHAR)
```

#### Description

Shortest path(s) using Dijkstra's algorithm on a graph to which points located on the edges are added as temporary vertices.

`pgr_withPoints(edges, points, start vids, end vids, [driving_side], [directed := true, details := false])`

The edges are a table-valued argument with the columns `id`, `source`, `target`, `cost` and optionally `reverse_cost`, as for `pgr_dijkstra`.

The points are a constant list of structs with the fields:

| Field | Type | Description |
| --- | --- | --- |
| `pid` | integer | Optional. Identifier of the point, which becomes the vertex `-pid`. Defaults to the position in the list, starting from 1 |
| `edge_id` | integer | Identifier of the edge the point is on. Points on unknown edges are ignored |
| `fraction` | numeric | Position on the edge, between 0 (at `source`) and 1 (at `target`) |
| `side` | VARCHAR | Optional. `r`, `l` or `b` (default, also used for NULL): the side of the edge the point is on, looking from `source` to `target` |

A table-valued argument can only be used once per call, so the list has to be built beforehand, for example with `SET VARIABLE points = (SELECT list(p) FROM points_of_interest p)` and passed as `getvariable('points')`.

`start vids` and `end vids` are a single integer or a list of integers. Negative values designate points, positive values vertices of the graph.

`driving_side` is `r` (default on a directed graph), `l` or `b` (always used on an undirected graph). With right side driving a point on the right side of an edge can only be reached while travelling from `source` to `target`, and a point on the left side while travelling from `target` to `source`; left side driving is the opposite. It can be given as the fifth positional argument or by name.

When `details` is false, consecutive rows of a path that are on the same edge are merged, which hides the points that are passed along the way. When it is true every point that is passed is returned as a row with a negative `node`.

The result has the columns `seq`, `path_seq`, `start_vid`, `end_vid`, `node`, `edge`, `cost` and `agg_cost` of `pgr_dijkstra`, ordered by `start_vid` and `end_vid`.

Differences with pgRouting: the edges are a table-valued argument and the points a list of structs instead of two SQL strings, and the combinations signature is not available.

#### Example

```sql
SET VARIABLE points = (SELECT list(p) FROM (SELECT pid, edge_id, fraction, side FROM points_of_interest) p);

SELECT * FROM pgr_withPoints((SELECT id, source, target, cost, reverse_cost FROM edges), getvariable('points'), -1, [10, -3], 'r', details := true);
```

----

### ST_AddEdgeModFace

#### Signature

```sql
ST_AddEdgeModFace (col0 VARCHAR, col1 INTEGER, col2 INTEGER, col3 ANY)
```

#### Description

Adds an edge between two existing nodes and returns its id. If the edge splits a face, the face is kept for one side and a new face is added for the other.

`ST_AddEdgeModFace(toponame, start_node, end_node, line)`. The new face is created on the left of the new edge whenever that side is bounded, and on the right otherwise. The links of the adjacent edges are updated, as are the `left_face` / `right_face` of the edges, the `containing_face` of the isolated nodes that end up in the new face, and the bounding box of the modified face.

Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- invalid curve`, `- curve not simple`, `- non-existent node`, `- start node not geometry start point.`, `- end node not geometry end point.`, `- geometry crosses a node`, `- geometry crosses edge N`, `- coincident edge N`, `Spatial exception - geometry intersects edge N`, `Geometry SRID (...) does not match topology SRID (...)`.

The function runs in its own transaction, on a separate connection to the same database. The change is committed as soon as the call returns, independently of the transaction of the caller: a later `ROLLBACK` does not undo it. If the call fails nothing is changed. It cannot see uncommitted changes either. Calling it inside an explicit transaction (`BEGIN`) that already holds uncommitted changes to the database therefore raises an error asking to commit or roll back first; the check covers any change to the database, not only changes to the tables of the topology, because DuckDB does not expose which tables a transaction has updated or deleted from. A conflict with a concurrent transaction that modified the same topology rows is reported as an error too. Inside an explicit transaction the caller may not see the change in the topology tables until it starts a new transaction. Concurrent edits of one database are executed one after the other.

Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`; subqueries and column references are not allowed. Geometry arguments take a `GEOMETRY` (or its WKT text); a geometry whose type carries a coordinate system must match the SRID of the topology.

#### Example

```sql
CALL CreateTopology('city');
SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText(
    'GEOMETRYCOLLECTION(POLYGON((0 0, 10 0, 10 10, 0 10, 0 0)), POINT(5 0), POINT(5 10))'));
SET VARIABLE a = (SELECT node_id FROM city.node WHERE ST_Equals(geom, ST_Point(5, 0)));
SET VARIABLE b = (SELECT node_id FROM city.node WHERE ST_Equals(geom, ST_Point(5, 10)));
SELECT * FROM ST_AddEdgeModFace('city', getvariable('a'), getvariable('b'),
    ST_GeomFromText('LINESTRING(5 0, 5 10)'));
SELECT count(*) FROM city.face WHERE face_id <> 0;
-- 2
```

----

### ST_AddEdgeNewFaces

#### Signature

```sql
ST_AddEdgeNewFaces (col0 VARCHAR, col1 INTEGER, col2 INTEGER, col3 ANY)
```

#### Description

Adds an edge between two existing nodes and returns its id. If the edge splits a face, the face is deleted and replaced by two new faces.

`ST_AddEdgeNewFaces(toponame, start_node, end_node, line)`. The links of the adjacent edges are updated, as are the `left_face` / `right_face` of every edge and the `containing_face` of every isolated node of the split face. When the universal face is split only the bounded side gets a new face.

Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- invalid curve`, `- curve not simple`, `- non-existent node`, `- start node not geometry start point.`, `- end node not geometry end point.`, `- geometry crosses a node`, `- geometry crosses edge N`, `- coincident edge N`, `Spatial exception - geometry intersects edge N`, `Geometry SRID (...) does not match topology SRID (...)`.

The function runs in its own transaction, on a separate connection to the same database. The change is committed as soon as the call returns, independently of the transaction of the caller: a later `ROLLBACK` does not undo it. If the call fails nothing is changed. It cannot see uncommitted changes either. Calling it inside an explicit transaction (`BEGIN`) that already holds uncommitted changes to the database therefore raises an error asking to commit or roll back first; the check covers any change to the database, not only changes to the tables of the topology, because DuckDB does not expose which tables a transaction has updated or deleted from. A conflict with a concurrent transaction that modified the same topology rows is reported as an error too. Inside an explicit transaction the caller may not see the change in the topology tables until it starts a new transaction. Concurrent edits of one database are executed one after the other.

Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`; subqueries and column references are not allowed. Geometry arguments take a `GEOMETRY` (or its WKT text); a geometry whose type carries a coordinate system must match the SRID of the topology.

#### Example

```sql
CALL CreateTopology('city');
SELECT * FROM ST_AddIsoNode('city', NULL, ST_Point(0, 0));
SELECT * FROM ST_AddEdgeNewFaces('city', 1, 1, ST_GeomFromText('LINESTRING(0 0, 10 0, 10 10, 0 10, 0 0)'));
SELECT face_id FROM city.face;
-- 0 and 1
```

----

### ST_AddIsoEdge

#### Signature

```sql
ST_AddIsoEdge (col0 VARCHAR, col1 INTEGER, col2 INTEGER, col3 ANY)
```

#### Description

Adds an isolated edge between two isolated nodes of the same face and returns its id.

`ST_AddIsoEdge(toponame, start_node, end_node, line)`. Both nodes stop being isolated (`containing_face` becomes NULL).

Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- invalid curve`, `- curve not simple`, `- non-existent node`, `- not isolated node`, `- nodes in different faces`, `- start node not geometry start point.`, `- end node not geometry end point.`, `- geometry crosses a node`, `- geometry crosses edge N`, `- coincident edge N`, `Spatial exception - geometry intersects edge N`, and `Closed edges would not be isolated, try ST_AddEdgeNewFaces` when both nodes are the same.

The function runs in its own transaction, on a separate connection to the same database. The change is committed as soon as the call returns, independently of the transaction of the caller: a later `ROLLBACK` does not undo it. If the call fails nothing is changed. It cannot see uncommitted changes either. Calling it inside an explicit transaction (`BEGIN`) that already holds uncommitted changes to the database therefore raises an error asking to commit or roll back first; the check covers any change to the database, not only changes to the tables of the topology, because DuckDB does not expose which tables a transaction has updated or deleted from. A conflict with a concurrent transaction that modified the same topology rows is reported as an error too. Inside an explicit transaction the caller may not see the change in the topology tables until it starts a new transaction. Concurrent edits of one database are executed one after the other.

Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`; subqueries and column references are not allowed. Geometry arguments take a `GEOMETRY` (or its WKT text); a geometry whose type carries a coordinate system must match the SRID of the topology.

#### Example

```sql
CALL CreateTopology('city');
SELECT * FROM ST_AddIsoNode('city', NULL, ST_Point(0, 0));
SELECT * FROM ST_AddIsoNode('city', NULL, ST_Point(5, 0));
SELECT * FROM ST_AddIsoEdge('city', 1, 2, ST_GeomFromText('LINESTRING(0 0, 5 0)'));
-- 1
```

----

### ST_AddIsoNode

#### Signature

```sql
ST_AddIsoNode (col0 VARCHAR, col1 INTEGER, col2 ANY)
```

#### Description

Adds an isolated node to a face of a topology and returns its id.

`ST_AddIsoNode(toponame, face, point)`: if `face` is NULL the face containing the point is computed, otherwise the point must lie in that face.

Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- invalid point`, `- coincident node` (a node already exists at that location), `- edge crosses node.` (the point lies on an edge), `- not within face` (the point is not in the given face), `Geometry SRID (...) does not match topology SRID (...)`.

The function runs in its own transaction, on a separate connection to the same database. The change is committed as soon as the call returns, independently of the transaction of the caller: a later `ROLLBACK` does not undo it. If the call fails nothing is changed. It cannot see uncommitted changes either. Calling it inside an explicit transaction (`BEGIN`) that already holds uncommitted changes to the database therefore raises an error asking to commit or roll back first; the check covers any change to the database, not only changes to the tables of the topology, because DuckDB does not expose which tables a transaction has updated or deleted from. A conflict with a concurrent transaction that modified the same topology rows is reported as an error too. Inside an explicit transaction the caller may not see the change in the topology tables until it starts a new transaction. Concurrent edits of one database are executed one after the other.

Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`; subqueries and column references are not allowed. Geometry arguments take a `GEOMETRY` (or its WKT text); a geometry whose type carries a coordinate system must match the SRID of the topology.

#### Example

```sql
CALL CreateTopology('city');
SELECT * FROM ST_AddIsoNode('city', NULL, ST_Point(1, 1));
-- 1
```

----

### ST_ChangeEdgeGeom

#### Signature

```sql
ST_ChangeEdgeGeom (col0 VARCHAR, col1 INTEGER, col2 ANY)
```

#### Description

Changes the shape of an edge without changing the structure of the topology. Returns the text `Edge N changed`.

`ST_ChangeEdgeGeom(toponame, edge, line)`: the new line must keep the end points of the edge, must not meet any other edge or node, and must not sweep over a node or change the order of the edges around its end nodes. The bounding boxes of the faces on both sides are updated.

Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- invalid curve`, `- curve not simple`, `- non-existent edge N`, `- start node not geometry start point.`, `- end node not geometry end point.`, `- geometry crosses a node`, `- geometry crosses edge N`, `- coincident edge N`, `Spatial exception - geometry intersects edge N`, `Edge twist at node POINT(x y)`, `Edge motion collision at POINT(x y)`, `Edge changed disposition around start node N`, `Edge changed disposition around end node N`, `Edge ring changes winding`.

The function runs in its own transaction, on a separate connection to the same database. The change is committed as soon as the call returns, independently of the transaction of the caller: a later `ROLLBACK` does not undo it. If the call fails nothing is changed. It cannot see uncommitted changes either. Calling it inside an explicit transaction (`BEGIN`) that already holds uncommitted changes to the database therefore raises an error asking to commit or roll back first; the check covers any change to the database, not only changes to the tables of the topology, because DuckDB does not expose which tables a transaction has updated or deleted from. A conflict with a concurrent transaction that modified the same topology rows is reported as an error too. Inside an explicit transaction the caller may not see the change in the topology tables until it starts a new transaction. Concurrent edits of one database are executed one after the other.

Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`; subqueries and column references are not allowed. Geometry arguments take a `GEOMETRY` (or its WKT text); a geometry whose type carries a coordinate system must match the SRID of the topology.

#### Example

```sql
CALL CreateTopology('city');
SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText('LINESTRING(0 0, 10 0)'));
SELECT * FROM ST_ChangeEdgeGeom('city', 1, ST_GeomFromText('LINESTRING(0 0, 5 2, 10 0)'));
-- Edge 1 changed
```

----

### ST_CreateTopoGeo

#### Signature

```sql
ST_CreateTopoGeo (col0 VARCHAR, col1 ANY)
```

#### Description

Populates an empty topology from a geometry collection and returns the text `Topology name populated`.

`ST_CreateTopoGeo(toponame, collection)` takes the lines and polygon boundaries of the collection, nodes them against each other, merges the result into maximal edges and cuts these at the input points and at the end points of the input lines. It then stores the nodes (input points that are on no edge become isolated nodes), the edges with their `next_left_edge` / `next_right_edge` links, and one face per bounded region with its `left_face` / `right_face` labels and bounding box. Z and M coordinates are dropped.

Errors: `SQL/MM Spatial exception - null argument`, `SQL/MM Spatial exception - invalid topology name`, `SQL/MM Spatial exception - non-empty view` (the topology already holds nodes or edges), `SQL/MM Spatial exception - non-empty face view`, `Geometry SRID (...) does not match topology SRID (...)`.

Differences from PostGIS: the whole topology is computed in memory and written at once instead of edge by edge, so identifiers are assigned in a different order (edges are sorted by their coordinates); a closed ring keeps a single node, placed on an input point if one lies on it.

The function runs in its own transaction, on a separate connection to the same database. The change is committed as soon as the call returns, independently of the transaction of the caller: a later `ROLLBACK` does not undo it. If the call fails nothing is changed. It cannot see uncommitted changes either. Calling it inside an explicit transaction (`BEGIN`) that already holds uncommitted changes to the database therefore raises an error asking to commit or roll back first; the check covers any change to the database, not only changes to the tables of the topology, because DuckDB does not expose which tables a transaction has updated or deleted from. A conflict with a concurrent transaction that modified the same topology rows is reported as an error too. Inside an explicit transaction the caller may not see the change in the topology tables until it starts a new transaction. Concurrent edits of one database are executed one after the other.

Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`; subqueries and column references are not allowed. Geometry arguments take a `GEOMETRY` (or its WKT text); a geometry whose type carries a coordinate system must match the SRID of the topology.

#### Example

```sql
CALL CreateTopology('city');
SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText(
    'GEOMETRYCOLLECTION(POLYGON((0 0, 10 0, 10 10, 0 10, 0 0)), LINESTRING(5 0, 5 10), POINT(2 2))'));
SELECT count(*) FROM city.face WHERE face_id <> 0;
-- 2
```

----

### ST_Drivers

#### Signature

```sql
ST_Drivers ()
```

#### Description

Returns the list of supported GDAL drivers and file formats

Note that far from all of these drivers have been tested properly.
Some may require additional options to be passed to work as expected.
If you run into any issues please first consult the [consult the GDAL docs](https://gdal.org/drivers/vector/index.html).

#### Example

```sql
SELECT * FROM ST_Drivers();
```

----

### ST_DumpPoints

#### Signature

```sql
ST_DumpPoints (col0 GEOMETRY)
```

#### Description

Extracts all vertices from a geometry as individual point geometries.

Returns a table with a 'geom' column containing each point and a 'path' column
(an integer array) showing the position of each vertex in the geometry tree.

#### Example

```sql
SELECT * FROM ST_DumpPoints('LINESTRING(0 0, 1 1, 2 2)'::GEOMETRY);
```

----

### ST_DumpRings

#### Signature

```sql
ST_DumpRings (col0 GEOMETRY)
```

#### Description

Extracts the rings of a polygon geometry.

Returns a table with a 'geom' column containing each ring as a linestring and
a 'path' column (integer) where 0 is the exterior ring and 1,2,... are interior rings (holes).
Only works on POLYGON geometries.

#### Example

```sql
SELECT * FROM ST_DumpRings('POLYGON((0 0, 1 0, 1 1, 0 1, 0 0), (0.25 0.25, 0.75 0.25, 0.75 0.75, 0.25 0.75, 0.25 0.25))'::GEOMETRY);
```

----

### ST_DumpSegments

#### Signature

```sql
ST_DumpSegments (col0 GEOMETRY)
```

#### Description

Extracts consecutive vertex pairs from a geometry as 2-point linestring segments.

Returns a table with a 'geom' column containing each segment as a linestring and
a 'path' column (integer) with the segment index.
For example, LINESTRING(0 0, 1 1, 2 2) produces LINESTRING(0 0, 1 1) and LINESTRING(1 1, 2 2).

#### Example

```sql
SELECT * FROM ST_DumpSegments('LINESTRING(0 0, 1 1, 2 2)'::GEOMETRY);
```

----

### ST_GDALDrivers

#### Signature

```sql
ST_GDALDrivers ()
```

#### Description

Returns the GDAL raster drivers that are built into the extension: the formats that `ST_ReadRaster` and `ST_FromGDALRaster` can read and that `ST_AsGDALRaster` can write.

`idx` is the position of the driver in GDAL's driver list, `can_read` and `can_write` tell whether the driver opens and creates files, and `create_options` is the XML description of the creation options that `ST_AsGDALRaster` accepts. The vector drivers are listed by `ST_Drivers()`.

#### Example

```sql
SELECT short_name, can_read, can_write FROM ST_GDALDrivers() ORDER BY short_name;
```

----

### ST_GeneratePoints

#### Signature

```sql
ST_GeneratePoints (col0 BOX_2D, col1 BIGINT)
ST_GeneratePoints (col0 BOX_2D, col1 BIGINT, col2 BIGINT)
```

#### Description

Generates a set of random points within the specified bounding box.

Takes a bounding box (min_x, min_y, max_x, max_y), a count of points to generate, and optionally a seed for the random number generator.

#### Example

```sql
SELECT * FROM ST_GeneratePoints({min_x: 0, min_y:0, max_x:10, max_y:10}::BOX_2D, 5, 42);
```

----

### ST_GetFaceEdges

#### Signature

```sql
ST_GetFaceEdges (col0 VARCHAR, col1 INTEGER)
```

#### Description

Returns the ordered set of signed edges bounding a face, as rows of `(sequence, edge)`.

Each ring is walked with the face on its left: an edge is positive when it is followed in its own direction, negative otherwise. The enumeration of a ring starts from its edge with the smallest identifier; the outer ring comes first, then the holes ordered by their smallest edge. Edges that have the face on both sides are not part of its boundary and are not returned. An unknown face yields no rows.

Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`.

The function reads through a separate connection and sees committed data only. Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`.

#### Example

```sql
CALL CreateTopology('city');
SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))'));
SELECT * FROM ST_GetFaceEdges('city', 1);
```

----

### ST_GetFaceGeometry

#### Signature

```sql
ST_GetFaceGeometry (col0 VARCHAR, col1 INTEGER)
```

#### Description

Returns the polygon of a face, built from the edges that have the face on exactly one side.

Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- universal face has no geometry`, `- non-existent face.`. The result is an untyped `GEOMETRY` even when the topology has a SRID.

The function reads through a separate connection and sees committed data only. Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`.

#### Example

```sql
CALL CreateTopology('city');
SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))'));
SELECT ST_AsText(st_getfacegeometry) FROM ST_GetFaceGeometry('city', 1);
-- POLYGON ((0 0, 0 10, 10 10, 10 0, 0 0))
```

----

### ST_HexagonGrid

#### Signature

```sql
ST_HexagonGrid (col0 DOUBLE, col1 GEOMETRY)
```

#### Description

Generates a regular hexagonal grid covering the bounding box of the input geometry.

Takes an edge length (size) and a geometry whose bounding box defines the grid extent.
The grid is aligned to global coordinates, with cell (0,0) centered at the origin.
Returns rows of (geom, i, j) where geom is a flat-top hexagon polygon and (i, j) are the column and row indices.
Odd columns are offset vertically by half the row spacing.

#### Example

```sql
SELECT * FROM ST_HexagonGrid(1.0, ST_MakeEnvelope(0, 0, 3, 3));
```

----

### ST_ModEdgeHeal

#### Signature

```sql
ST_ModEdgeHeal (col0 VARCHAR, col1 INTEGER, col2 INTEGER)
```

#### Description

Heals two edges by deleting the node connecting them, modifying the first edge and deleting the second. Returns the id of the deleted node.

`ST_ModEdgeHeal(toponame, edge, other_edge)`: the first edge keeps its id and direction and takes over the geometry of both.

Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- non-existent edge N`, `- non-connected edges`, `- other edges connected (ids)` when the shared node has other edges, `Cannot heal edge N with itself, try with another`, `Edge N is closed, cannot heal to edge M`.

The function runs in its own transaction, on a separate connection to the same database. The change is committed as soon as the call returns, independently of the transaction of the caller: a later `ROLLBACK` does not undo it. If the call fails nothing is changed. It cannot see uncommitted changes either. Calling it inside an explicit transaction (`BEGIN`) that already holds uncommitted changes to the database therefore raises an error asking to commit or roll back first; the check covers any change to the database, not only changes to the tables of the topology, because DuckDB does not expose which tables a transaction has updated or deleted from. A conflict with a concurrent transaction that modified the same topology rows is reported as an error too. Inside an explicit transaction the caller may not see the change in the topology tables until it starts a new transaction. Concurrent edits of one database are executed one after the other.

Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`; subqueries and column references are not allowed. Geometry arguments take a `GEOMETRY` (or its WKT text); a geometry whose type carries a coordinate system must match the SRID of the topology.

#### Example

```sql
CALL CreateTopology('city');
SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText('GEOMETRYCOLLECTION(LINESTRING(0 0, 10 0), POINT(4 0))'));
SELECT * FROM ST_ModEdgeHeal('city', 1, 2);
```

----

### ST_ModEdgeSplit

#### Signature

```sql
ST_ModEdgeSplit (col0 VARCHAR, col1 INTEGER, col2 ANY)
```

#### Description

Splits an edge by creating a node on it, modifying the original edge and adding a new one. Returns the id of the new node.

`ST_ModEdgeSplit(toponame, edge, point)`: the original edge keeps its id and now ends at the new node; the new edge runs from the new node to the old end node.

Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- invalid point`, `- non-existent edge`, `- coincident node`, `- point not on edge`.

The function runs in its own transaction, on a separate connection to the same database. The change is committed as soon as the call returns, independently of the transaction of the caller: a later `ROLLBACK` does not undo it. If the call fails nothing is changed. It cannot see uncommitted changes either. Calling it inside an explicit transaction (`BEGIN`) that already holds uncommitted changes to the database therefore raises an error asking to commit or roll back first; the check covers any change to the database, not only changes to the tables of the topology, because DuckDB does not expose which tables a transaction has updated or deleted from. A conflict with a concurrent transaction that modified the same topology rows is reported as an error too. Inside an explicit transaction the caller may not see the change in the topology tables until it starts a new transaction. Concurrent edits of one database are executed one after the other.

Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`; subqueries and column references are not allowed. Geometry arguments take a `GEOMETRY` (or its WKT text); a geometry whose type carries a coordinate system must match the SRID of the topology.

#### Example

```sql
CALL CreateTopology('city');
SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText('LINESTRING(0 0, 10 0)'));
SELECT * FROM ST_ModEdgeSplit('city', 1, ST_Point(4, 0));
-- 3
```

----

### ST_MoveIsoNode

#### Signature

```sql
ST_MoveIsoNode (col0 VARCHAR, col1 INTEGER, col2 ANY)
```

#### Description

Moves an isolated node to another location within its face. Returns the text `Isolated Node N moved to location x,y`.

Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- invalid point`, `- non-existent node`, `- not isolated node`, `- coincident node`, `- edge crosses node.`, `Cannot move isolated node across faces`.

The function runs in its own transaction, on a separate connection to the same database. The change is committed as soon as the call returns, independently of the transaction of the caller: a later `ROLLBACK` does not undo it. If the call fails nothing is changed. It cannot see uncommitted changes either. Calling it inside an explicit transaction (`BEGIN`) that already holds uncommitted changes to the database therefore raises an error asking to commit or roll back first; the check covers any change to the database, not only changes to the tables of the topology, because DuckDB does not expose which tables a transaction has updated or deleted from. A conflict with a concurrent transaction that modified the same topology rows is reported as an error too. Inside an explicit transaction the caller may not see the change in the topology tables until it starts a new transaction. Concurrent edits of one database are executed one after the other.

Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`; subqueries and column references are not allowed. Geometry arguments take a `GEOMETRY` (or its WKT text); a geometry whose type carries a coordinate system must match the SRID of the topology.

#### Example

```sql
CALL CreateTopology('city');
SELECT * FROM ST_AddIsoNode('city', NULL, ST_Point(1, 1));
SELECT * FROM ST_MoveIsoNode('city', 1, ST_Point(2, 3));
-- Isolated Node 1 moved to location 2,3
```

----

### ST_NewEdgeHeal

#### Signature

```sql
ST_NewEdgeHeal (col0 VARCHAR, col1 INTEGER, col2 INTEGER)
```

#### Description

Heals two edges by deleting the node connecting them and replacing both edges with a new one, which has the direction of the first edge. Returns the id of the new edge.

Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- non-existent edge N`, `- non-connected edges`, `- other edges connected (ids)` when the shared node has other edges, `Cannot heal edge N with itself, try with another`, `Edge N is closed, cannot heal to edge M`.

The function runs in its own transaction, on a separate connection to the same database. The change is committed as soon as the call returns, independently of the transaction of the caller: a later `ROLLBACK` does not undo it. If the call fails nothing is changed. It cannot see uncommitted changes either. Calling it inside an explicit transaction (`BEGIN`) that already holds uncommitted changes to the database therefore raises an error asking to commit or roll back first; the check covers any change to the database, not only changes to the tables of the topology, because DuckDB does not expose which tables a transaction has updated or deleted from. A conflict with a concurrent transaction that modified the same topology rows is reported as an error too. Inside an explicit transaction the caller may not see the change in the topology tables until it starts a new transaction. Concurrent edits of one database are executed one after the other.

Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`; subqueries and column references are not allowed. Geometry arguments take a `GEOMETRY` (or its WKT text); a geometry whose type carries a coordinate system must match the SRID of the topology.

#### Example

```sql
CALL CreateTopology('city');
SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText('GEOMETRYCOLLECTION(LINESTRING(0 0, 10 0), POINT(4 0))'));
SELECT * FROM ST_NewEdgeHeal('city', 1, 2);
-- 3
```

----

### ST_NewEdgesSplit

#### Signature

```sql
ST_NewEdgesSplit (col0 VARCHAR, col1 INTEGER, col2 ANY)
```

#### Description

Splits an edge by creating a node on it, deleting the original edge and replacing it with two new edges. Returns the id of the new node.

`ST_NewEdgesSplit(toponame, edge, point)`: the first new edge runs from the old start node to the new node, the second from the new node to the old end node.

Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- invalid point`, `- non-existent edge`, `- coincident node`, `- point not on edge`.

The function runs in its own transaction, on a separate connection to the same database. The change is committed as soon as the call returns, independently of the transaction of the caller: a later `ROLLBACK` does not undo it. If the call fails nothing is changed. It cannot see uncommitted changes either. Calling it inside an explicit transaction (`BEGIN`) that already holds uncommitted changes to the database therefore raises an error asking to commit or roll back first; the check covers any change to the database, not only changes to the tables of the topology, because DuckDB does not expose which tables a transaction has updated or deleted from. A conflict with a concurrent transaction that modified the same topology rows is reported as an error too. Inside an explicit transaction the caller may not see the change in the topology tables until it starts a new transaction. Concurrent edits of one database are executed one after the other.

Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`; subqueries and column references are not allowed. Geometry arguments take a `GEOMETRY` (or its WKT text); a geometry whose type carries a coordinate system must match the SRID of the topology.

#### Example

```sql
CALL CreateTopology('city');
SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText('LINESTRING(0 0, 10 0)'));
SELECT * FROM ST_NewEdgesSplit('city', 1, ST_Point(4, 0));
SELECT edge_id FROM city.edge ORDER BY edge_id;
-- 2 and 3
```

----

### ST_Read

#### Signature

```sql
ST_Read (col0 VARCHAR, keep_wkb BOOLEAN, max_batch_size INTEGER, layer VARCHAR, sibling_files VARCHAR[], allowed_drivers VARCHAR[], open_options VARCHAR[])
```

#### Description

Read and import a variety of geospatial file formats using the GDAL library.

The `ST_Read` table function is based on the [GDAL](https://gdal.org/index.html) translator library and enables reading spatial data from a variety of geospatial vector file formats as if they were DuckDB tables.

> See [ST_Drivers](#st_drivers) for a list of supported file formats and drivers.

Except for the `path` parameter, all parameters are optional.

| Parameter | Type | Description |
| --------- | -----| ----------- |
| `path` | VARCHAR | The path to the file to read. Mandatory |
| `sequential_layer_scan` | BOOLEAN | If set to true, the table function will scan through all layers sequentially and return the first layer that matches the given layer name. This is required for some drivers to work properly, e.g., the OSM driver. |
| `spatial_filter` | WKB_BLOB | If set to a WKB blob, the table function will only return rows that intersect with the given WKB geometry. Some drivers may support efficient spatial filtering natively, in which case it will be pushed down. Otherwise the filtering is done by GDAL which may be much slower. |
| `open_options` | VARCHAR[] | A list of key-value pairs that are passed to the GDAL driver to control the opening of the file. E.g., the GeoJSON driver supports a FLATTEN_NESTED_ATTRIBUTES=YES option to flatten nested attributes. |
| `layer` | VARCHAR | The name of the layer to read from the file. If NULL, the first layer is returned. Can also be a layer index (starting at 0). |
| `allowed_drivers` | VARCHAR[] | A list of GDAL driver names that are allowed to be used to open the file. If empty, all drivers are allowed. |
| `sibling_files` | VARCHAR[] | A list of sibling files that are required to open the file. E.g., the ESRI Shapefile driver requires a .shx file to be present. Although most of the time these can be discovered automatically. |
| `spatial_filter_box` | BOX_2D | If set to a BOX_2D, the table function will only return rows that intersect with the given bounding box. Similar to spatial_filter. |
| `keep_wkb` | BOOLEAN | If set, the table function will return geometries in a wkb_geometry column with the type WKB_BLOB (which can be cast to BLOB) instead of GEOMETRY. This is useful if you want to use DuckDB with more exotic geometry subtypes that DuckDB spatial doesn't support representing in the GEOMETRY type yet. |

Note that GDAL is single-threaded, so this table function will not be able to make full use of parallelism.

By using `ST_Read`, the spatial extension also provides “replacement scans” for common geospatial file formats, allowing you to query files of these formats as if they were tables directly.

```sql
SELECT * FROM './path/to/some/shapefile/dataset.shp';
```

In practice this is just syntax-sugar for calling ST_Read, so there is no difference in performance. If you want to pass additional options, you should use the ST_Read table function directly.

The following formats are currently recognized by their file extension:

| Format | Extension |
| ------ | --------- |
| ESRI ShapeFile | .shp |
| GeoPackage | .gpkg |
| FlatGeoBuf | .fgb |

#### Example

```sql
-- Read a Shapefile
SELECT * FROM ST_Read('some/file/path/filename.shp');

-- Read a GeoJSON file
CREATE TABLE my_geojson_table AS SELECT * FROM ST_Read('some/file/path/filename.json');
```

----

### ST_Read_Meta

#### Signature

```sql
ST_Read_Meta (col0 VARCHAR)
ST_Read_Meta (col0 VARCHAR[])
```

#### Description

Read the metadata from a variety of geospatial file formats using the GDAL library.

The `ST_Read_Meta` table function accompanies the `ST_Read` table function, but instead of reading the contents of a file, this function scans the metadata instead.
Since the data model of the underlying GDAL library is quite flexible, most of the interesting metadata is within the returned `layers` column, which is a somewhat complex nested structure of DuckDB `STRUCT` and `LIST` types.

#### Example

```sql
-- Find the coordinate reference system authority name and code for the first layers first geometry column in the file
SELECT
    layers[1].geometry_fields[1].crs.auth_name as name,
    layers[1].geometry_fields[1].crs.auth_code as code
FROM st_read_meta('../../tmp/data/amsterdam_roads.fgb');
```

----

### ST_ReadOSM

#### Signature

```sql
ST_ReadOSM (col0 VARCHAR)
```

#### Description

The `ST_ReadOsm()` table function enables reading compressed OpenStreetMap data directly from a `.osm.pbf` file.

This function uses multithreading and zero-copy protobuf parsing which makes it a lot faster than using the `ST_Read()` OSM driver, however it only outputs the raw OSM data (Nodes, Ways, Relations), without constructing any geometries. For simple node entities (like PoI's) you can trivially construct POINT geometries, but it is also possible to construct LINESTRING and POLYGON geometries by manually joining refs and nodes together in SQL, although with available memory usually being a limiting factor.
The `ST_ReadOSM()` function also provides a "replacement scan" to enable reading from a file directly as if it were a table. This is just syntax sugar for calling `ST_ReadOSM()` though. Example:

```sql
SELECT * FROM 'tmp/data/germany.osm.pbf' LIMIT 5;
```

#### Example

```sql
SELECT *
FROM ST_ReadOSM('tmp/data/germany.osm.pbf')
WHERE tags['highway'] != []
LIMIT 5;
----
┌──────────────────────┬────────┬──────────────────────┬─────────┬────────────────────┬────────────┬───────────┬────────────────────────┐
│         kind         │   id   │         tags         │  refs   │        lat         │    lon     │ ref_roles │       ref_types        │
│ enum('node', 'way'…  │ int64  │ map(varchar, varch…  │ int64[] │       double       │   double   │ varchar[] │ enum('node', 'way', …  │
├──────────────────────┼────────┼──────────────────────┼─────────┼────────────────────┼────────────┼───────────┼────────────────────────┤
│ node                 │ 122351 │ {bicycle=yes, butt…  │         │         53.5492951 │   9.977553 │           │                        │
│ node                 │ 122397 │ {crossing=no, high…  │         │ 53.520990100000006 │ 10.0156924 │           │                        │
│ node                 │ 122493 │ {TMC:cid_58:tabcd_…  │         │ 53.129614600000004 │  8.1970173 │           │                        │
│ node                 │ 123566 │ {highway=traffic_s…  │         │ 54.617268200000005 │  8.9718171 │           │                        │
│ node                 │ 125801 │ {TMC:cid_58:tabcd_…  │         │ 53.070685000000005 │  8.7819939 │           │                        │
└──────────────────────┴────────┴──────────────────────┴─────────┴────────────────────┴────────────┴───────────┴────────────────────────┘
```

----

### ST_ReadRaster

#### Signature

```sql
ST_ReadRaster (col0 VARCHAR)
ST_ReadRaster (col0 VARCHAR, col1 INTEGER, col2 INTEGER)
```

#### Description

Reads a raster file, or every file matching a glob pattern, and returns its pixels as `RASTER` values.

Without a tile size the function returns one row per file holding the whole raster. With `tile_width` and `tile_height` (in pixels) each file is cut into tiles of that size, one row per tile; the tiles on the right and bottom edges are smaller when the raster size is not a multiple of the tile size. `x` and `y` are the column and row of the tile in the tile grid, starting at 0, and every tile carries its own georeference.

Any format of the bundled GDAL raster drivers can be read (see `ST_GDALDrivers()`; GeoTIFF, Cloud Optimized GeoTIFF, VRT and Erdas Imagine `.img`). Files are opened through DuckDB's file system, so remote paths (`https://`, `s3://`, ...) work as they do for `ST_Read`. All bands of a `RASTER` share the pixel type of the first band, colour tables are dropped and side-car files (`.tfw`, `.aux.xml`, `.ovr`) are not read. Tiles are read in parallel and are not returned in order.

#### Example

```sql
SELECT x, y, ST_Width(rast), ST_Height(rast) FROM ST_ReadRaster('test/data/raster/dem.tif', 16, 16) ORDER BY y, x;
```

----

### ST_ReadSHP

#### Signature

```sql
ST_ReadSHP (col0 VARCHAR, encoding VARCHAR)
```

#### Description

Read a Shapefile without relying on the GDAL library

----

### ST_RemEdgeModFace

#### Signature

```sql
ST_RemEdgeModFace (col0 VARCHAR, col1 INTEGER)
```

#### Description

Removes an edge. If it separates two faces, one is deleted and the other is modified to cover both.

`ST_RemEdgeModFace(toponame, edge)` returns the id of the face that remains in place of the edge. The face on the right of the edge is kept, unless one side is the universal face, which then absorbs the other. End nodes left without edges become isolated nodes of that face.

Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- non-existent edge N`.

The function runs in its own transaction, on a separate connection to the same database. The change is committed as soon as the call returns, independently of the transaction of the caller: a later `ROLLBACK` does not undo it. If the call fails nothing is changed. It cannot see uncommitted changes either. Calling it inside an explicit transaction (`BEGIN`) that already holds uncommitted changes to the database therefore raises an error asking to commit or roll back first; the check covers any change to the database, not only changes to the tables of the topology, because DuckDB does not expose which tables a transaction has updated or deleted from. A conflict with a concurrent transaction that modified the same topology rows is reported as an error too. Inside an explicit transaction the caller may not see the change in the topology tables until it starts a new transaction. Concurrent edits of one database are executed one after the other.

Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`; subqueries and column references are not allowed. Geometry arguments take a `GEOMETRY` (or its WKT text); a geometry whose type carries a coordinate system must match the SRID of the topology.

#### Example

```sql
CALL CreateTopology('city');
SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText(
    'GEOMETRYCOLLECTION(POLYGON((0 0, 10 0, 10 10, 0 10, 0 0)), LINESTRING(5 0, 5 10))'));
SET VARIABLE middle = (SELECT edge_id FROM city.edge WHERE ST_Intersects(geom, ST_Point(5, 5)));
SELECT * FROM ST_RemEdgeModFace('city', getvariable('middle'));
SELECT count(*) FROM city.face WHERE face_id <> 0;
-- 1
```

----

### ST_RemEdgeNewFace

#### Signature

```sql
ST_RemEdgeNewFace (col0 VARCHAR, col1 INTEGER)
```

#### Description

Removes an edge. If it separates two faces, both are deleted and replaced by a new face covering them.

`ST_RemEdgeNewFace(toponame, edge)` returns the id of the new face, or NULL when no face is created: the edge had the same face on both sides, or one of its sides was the universal face (which then absorbs the other). End nodes left without edges become isolated nodes of the resulting face.

Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- non-existent edge N`.

The function runs in its own transaction, on a separate connection to the same database. The change is committed as soon as the call returns, independently of the transaction of the caller: a later `ROLLBACK` does not undo it. If the call fails nothing is changed. It cannot see uncommitted changes either. Calling it inside an explicit transaction (`BEGIN`) that already holds uncommitted changes to the database therefore raises an error asking to commit or roll back first; the check covers any change to the database, not only changes to the tables of the topology, because DuckDB does not expose which tables a transaction has updated or deleted from. A conflict with a concurrent transaction that modified the same topology rows is reported as an error too. Inside an explicit transaction the caller may not see the change in the topology tables until it starts a new transaction. Concurrent edits of one database are executed one after the other.

Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`; subqueries and column references are not allowed. Geometry arguments take a `GEOMETRY` (or its WKT text); a geometry whose type carries a coordinate system must match the SRID of the topology.

#### Example

```sql
CALL CreateTopology('city');
SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText(
    'GEOMETRYCOLLECTION(POLYGON((0 0, 10 0, 10 10, 0 10, 0 0)), LINESTRING(5 0, 5 10))'));
SET VARIABLE middle = (SELECT edge_id FROM city.edge WHERE ST_Intersects(geom, ST_Point(5, 5)));
SELECT * FROM ST_RemEdgeNewFace('city', getvariable('middle'));
SELECT count(*) FROM city.face WHERE face_id <> 0;
-- 1
```

----

### ST_RemoveIsoEdge

#### Signature

```sql
ST_RemoveIsoEdge (col0 VARCHAR, col1 INTEGER)
```

#### Description

Removes an isolated edge. Its end nodes become isolated nodes of the face the edge was in. Returns the text `Isolated edge N removed`.

Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- non-existent edge`, `- not isolated edge` (the edge is closed, bounds a face, or shares a node with another edge).

The function runs in its own transaction, on a separate connection to the same database. The change is committed as soon as the call returns, independently of the transaction of the caller: a later `ROLLBACK` does not undo it. If the call fails nothing is changed. It cannot see uncommitted changes either. Calling it inside an explicit transaction (`BEGIN`) that already holds uncommitted changes to the database therefore raises an error asking to commit or roll back first; the check covers any change to the database, not only changes to the tables of the topology, because DuckDB does not expose which tables a transaction has updated or deleted from. A conflict with a concurrent transaction that modified the same topology rows is reported as an error too. Inside an explicit transaction the caller may not see the change in the topology tables until it starts a new transaction. Concurrent edits of one database are executed one after the other.

Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`; subqueries and column references are not allowed. Geometry arguments take a `GEOMETRY` (or its WKT text); a geometry whose type carries a coordinate system must match the SRID of the topology.

#### Example

```sql
CALL CreateTopology('city');
SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText('LINESTRING(0 0, 10 0)'));
SELECT * FROM ST_RemoveIsoEdge('city', 1);
-- Isolated edge 1 removed
```

----

### ST_RemoveIsoNode

#### Signature

```sql
ST_RemoveIsoNode (col0 VARCHAR, col1 INTEGER)
```

#### Description

Removes an isolated node. Returns the text `Isolated node N removed`.

Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`, `- non-existent node`, `- not isolated node`.

The function runs in its own transaction, on a separate connection to the same database. The change is committed as soon as the call returns, independently of the transaction of the caller: a later `ROLLBACK` does not undo it. If the call fails nothing is changed. It cannot see uncommitted changes either. Calling it inside an explicit transaction (`BEGIN`) that already holds uncommitted changes to the database therefore raises an error asking to commit or roll back first; the check covers any change to the database, not only changes to the tables of the topology, because DuckDB does not expose which tables a transaction has updated or deleted from. A conflict with a concurrent transaction that modified the same topology rows is reported as an error too. Inside an explicit transaction the caller may not see the change in the topology tables until it starts a new transaction. Concurrent edits of one database are executed one after the other.

Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`; subqueries and column references are not allowed. Geometry arguments take a `GEOMETRY` (or its WKT text); a geometry whose type carries a coordinate system must match the SRID of the topology.

#### Example

```sql
CALL CreateTopology('city');
SELECT * FROM ST_AddIsoNode('city', NULL, ST_Point(1, 1));
SELECT * FROM ST_RemoveIsoNode('city', 1);
-- Isolated node 1 removed
```

----

### ST_SquareGrid

#### Signature

```sql
ST_SquareGrid (col0 DOUBLE, col1 GEOMETRY)
```

#### Description

Generates a regular grid of square polygons covering the bounding box of the input geometry.

Takes a cell size and a geometry whose bounding box defines the grid extent.
The grid is aligned to global coordinates (multiples of size), so grid cell (0,0) always covers the origin.
Returns rows of (geom, i, j) where geom is the square polygon and (i, j) are the column and row indices.

#### Example

```sql
SELECT * FROM ST_SquareGrid(1.0, ST_MakeEnvelope(0, 0, 2, 2));
```

----

### TopologySummary

#### Signature

```sql
TopologySummary (col0 VARCHAR)
```

#### Description

Returns a two-line text summary of a topology: its id, SRID and precision, then the number of nodes, edges and faces (the universal face is not counted).

Errors: `SQL/MM Spatial exception - invalid topology name`. The TopoGeometry layer is not implemented, so the summary always reports `0 topogeoms in 0 layers`.

The function reads through a separate connection and sees committed data only. Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`.

#### Example

```sql
CALL CreateTopology('city');
SELECT * FROM TopologySummary('city');
-- Topology city (id 1, SRID 0, precision 0)
-- 0 nodes, 0 edges, 0 faces, 0 topogeoms in 0 layers
```

----

### ValidateTopology

#### Signature

```sql
ValidateTopology (col0 VARCHAR)
```

#### Description

Checks a topology and returns one `(error, id1, id2)` row per problem found; a valid topology yields no rows.

| error | id1 | id2 |
| --- | --- | --- |
| coincident nodes | node | node |
| edge crosses node | edge | node |
| invalid edge | edge | |
| edge not simple | edge | |
| edge crosses edge | edge | edge |
| edge start node geometry mismatch | edge | node |
| edge end node geometry mismatch | edge | node |
| face without edges | face | |
| invalid next_right_edge | edge | expected value |
| invalid next_left_edge | edge | expected value |
| mixed face labeling in ring | signed edge of the ring | |
| universal face has shell rings | 0 | signed edge of the ring |
| face has multiple shells | face | signed edge of the ring |
| face has no rings | face | |
| face has wrong mbr | face | |
| hole not in advertised face | signed edge of the ring | |
| not-isolated node has not-null containing_face | node | |
| isolated node has null containing_face | node | |
| isolated node has wrong containing_face | node | |
| face within face | inner face | outer face |
| face overlaps face | face | face |

The expected links and rings are derived from the geometry of the edges. The ring, face and isolated-node checks are skipped when one of the first seven kinds of error is reported, since they need sound linework.

Errors: `SQL/MM Spatial exception - null argument`, `- invalid topology name`. Unlike PostGIS there is no bounding-box argument: the whole topology is loaded and checked.

The function reads through a separate connection and sees committed data only. Arguments must be constants or expressions that can be folded at bind time, such as `getvariable('x')`.

#### Example

```sql
CALL CreateTopology('city');
SELECT * FROM ST_CreateTopoGeo('city', ST_GeomFromText('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))'));
SELECT * FROM ValidateTopology('city');
-- no rows
```

----

