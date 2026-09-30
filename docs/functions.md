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
| [`ST_AddMeasure`](#st_addmeasure) | Adds M values along a linestring, interpolated between start and end measures |
| [`ST_AddPoint`](#st_addpoint) | Adds a point to a linestring at a given position (default: end) |
| [`ST_Affine`](#st_affine) | Applies an affine transformation to a geometry. |
| [`ST_Angle`](#st_angle) | Returns the angle in radians between two points |
| [`ST_Area`](#st_area) | Compute the area of a geometry. |
| [`ST_Area_Spheroid`](#st_area_spheroid) | Returns the area of a geometry in meters, using an ellipsoidal model of the earth |
| [`ST_AsEncodedPolyline`](#st_asencodedpolyline) | Encodes a linestring as a Google Encoded Polyline string |
| [`ST_AsEWKB`](#st_asewkb) | Returns the geometry as EWKB (Extended Well-Known Binary). Alias for ST_AsWKB. |
| [`ST_AsEWKT`](#st_asewkt) | Returns the geometry as an Extended WKT (EWKT) string |
| [`ST_AsGeoJSON`](#st_asgeojson) | Returns the geometry as a GeoJSON fragment |
| [`ST_AsGML`](#st_asgml) | Returns the geometry as a GML (Geography Markup Language) element. |
| [`ST_AsHEXWKB`](#st_ashexwkb) | Returns the geometry as a HEXWKB string |
| [`ST_AsKML`](#st_askml) | Returns the geometry as a KML (Keyhole Markup Language) geometry element. |
| [`ST_AsLatLonText`](#st_aslatlontext) | Returns a point as a DMS (degrees-minutes-seconds) latitude/longitude string |
| [`ST_AsMVTGeom`](#st_asmvtgeom) | Transform and clip geometry to a tile boundary |
| [`ST_AsSVG`](#st_assvg) | Convert the geometry into a SVG fragment or path |
| [`ST_AsText`](#st_astext) | Returns the Well-Known Text (WKT) representation of the geometry |
| [`ST_AsTWKB`](#st_astwkb) | Encodes geometry as Tiny WKB (TWKB) with specified coordinate precision |
| [`ST_AsWKB`](#st_aswkb) | Returns the Well-Known Binary (WKB) representation of the geometry |
| [`ST_Azimuth`](#st_azimuth) | Returns the azimuth (a clockwise angle measured from north) of two points in radian. |
| [`ST_Boundary`](#st_boundary) | Returns the "boundary" of a geometry |
| [`ST_BoundingDiagonal`](#st_boundingdiagonal) | Returns the diagonal of the bounding box as a linestring |
| [`ST_Box2dFromGeoHash`](#st_box2dfromgeohash) | Returns the bounding box polygon of a GeoHash cell |
| [`ST_Buffer`](#st_buffer) | Returns a buffer around the input geometry at the target distance |
| [`ST_BuildArea`](#st_buildarea) | Creates a polygonal geometry by attempting to "fill in" the input geometry. |
| [`ST_Centroid`](#st_centroid) | Returns the centroid of a geometry |
| [`ST_ChaikinSmoothing`](#st_chaikinsmoothing) | Smooths a geometry using Chaikin's corner-cutting algorithm |
| [`ST_ClipByBox2D`](#st_clipbybox2d) | Clips a geometry by a bounding box |
| [`ST_ClosestPoint`](#st_closestpoint) | Returns the closest point on the first geometry to the second geometry |
| [`ST_Collect`](#st_collect) | Collects a list of geometries into a collection geometry. |
| [`ST_CollectionExtract`](#st_collectionextract) | Extracts geometries from a GeometryCollection into a typed multi geometry. |
| [`ST_ConcaveHull`](#st_concavehull) | Returns the 'concave' hull of the input geometry, containing all of the source input's points, and which can be used to create polygons from points. The ratio parameter dictates the level of concavity; 1.0 returns the convex hull; and 0 indicates to return the most concave hull possible. Set allowHoles to a non-zero value to allow output containing holes. |
| [`ST_Contains`](#st_contains) | Returns true if the first geometry contains the second geometry |
| [`ST_ContainsProperly`](#st_containsproperly) | Returns true if the first geometry \"properly\" contains the second geometry |
| [`ST_ConvexHull`](#st_convexhull) | Returns the convex hull enclosing the geometry |
| [`ST_CoordDim`](#st_coorddim) | Returns the coordinate dimension of a geometry |
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
| [`ST_GeogFromText`](#st_geogfromtext) | Creates a GEOGRAPHY from its WKT representation. |
| [`ST_GeogFromWKB`](#st_geogfromwkb) | Creates a GEOGRAPHY from its WKB representation. |
| [`ST_GeogFromWKT`](#st_geogfromwkt) | Creates a GEOGRAPHY from its WKT representation. |
| [`ST_GeogPoint`](#st_geogpoint) | Creates a GEOGRAPHY point from a longitude and a latitude in degrees on WGS84 |
| [`ST_GeographyFromText`](#st_geographyfromtext) | Creates a GEOGRAPHY from its WKT representation. |
| [`ST_GeoHash`](#st_geohash) | Returns the GeoHash string of a geometry's centroid at the given precision |
| [`ST_GeometricMedian`](#st_geometricmedian) | Returns the geometric median of a geometry's vertices (Weiszfeld algorithm) |
| [`ST_GeometryN`](#st_geometryn) | Returns the Nth geometry from a geometry collection (0-indexed) |
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
| [`ST_HasM`](#st_hasm) | Check if the input geometry has M values. |
| [`ST_HasZ`](#st_hasz) | Check if the input geometry has Z values. |
| [`ST_HausdorffDistance`](#st_hausdorffdistance) | Returns the Hausdorff distance between two geometries |
| [`ST_Hilbert`](#st_hilbert) | Encodes the X and Y values as the hilbert curve index for a curve covering the given bounding box. |
| [`ST_InteriorRingN`](#st_interiorringn) | Returns the N-th interior ring (hole) of a POLYGON as a LINESTRING. Indexing is 1-based  (n = 1 returns the first interior ring). Returns NULL if the polygon is empty or has fewer than N interior rings. |
| [`ST_InterpolatePoint`](#st_interpolatepoint) | Computes the closest point on a LINESTRING to a given POINT and returns the interpolated M value of that point. |
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
| [`ST_MakeEnvelope`](#st_makeenvelope) | Create a rectangular polygon from min/max coordinates |
| [`ST_MakeLine`](#st_makeline) | Create a LINESTRING from a list of POINT geometries |
| [`ST_MakePoint`](#st_makepoint) | Creates a GEOMETRY point from an pair of floating point numbers. |
| [`ST_MakePolygon`](#st_makepolygon) | Create a POLYGON from a LINESTRING shell |
| [`ST_MakeValid`](#st_makevalid) | Returns a valid representation of the geometry |
| [`ST_MaxDistance`](#st_maxdistance) | Returns the maximum distance between two geometries |
| [`ST_MaximumInscribedCircle`](#st_maximuminscribedcircle) | Returns the maximum inscribed circle of the input geometry, optionally with a tolerance. |
| [`ST_MemSize`](#st_memsize) | Returns the memory size of a geometry in bytes |
| [`ST_MinimumBoundingCircle`](#st_minimumboundingcircle) | Returns the minimum bounding circle of a geometry |
| [`ST_MinimumClearance`](#st_minimumclearance) | Returns the minimum clearance of a geometry |
| [`ST_MinimumClearanceLine`](#st_minimumclearanceline) | Returns the line spanning the minimum clearance |
| [`ST_MinimumRotatedRectangle`](#st_minimumrotatedrectangle) | Returns the minimum rotated rectangle that bounds the input geometry, finding the surrounding box that has the lowest area by using a rotated rectangle, rather than taking the lowest and highest coordinate values as per ST_Envelope(). |
| [`ST_MMax`](#st_mmax) | Returns the maximum M coordinate of a geometry |
| [`ST_MMin`](#st_mmin) | Returns the minimum M coordinate of a geometry |
| [`ST_Multi`](#st_multi) | Turns a single geometry into a multi geometry. |
| [`ST_NDims`](#st_ndims) | Returns the topological dimension of a geometry |
| [`ST_NGeometries`](#st_ngeometries) | Returns the number of component geometries in a collection geometry. |
| [`ST_NInteriorRings`](#st_ninteriorrings) | Returns the number of interior rings of a polygon |
| [`ST_Node`](#st_node) | Returns a "noded" MultiLinestring, produced by combining a collection of input linestrings and adding additional vertices where they intersect. |
| [`ST_Normalize`](#st_normalize) | Returns the "normalized" representation of the geometry |
| [`ST_NPoints`](#st_npoints) | Returns the number of vertices within a geometry |
| [`ST_NRings`](#st_nrings) | Returns the number of rings in a polygon (exterior + interior) |
| [`ST_NumGeometries`](#st_numgeometries) | Returns the number of component geometries in a collection geometry. |
| [`ST_NumInteriorRings`](#st_numinteriorrings) | Returns the number of interior rings of a polygon |
| [`ST_NumPoints`](#st_numpoints) | Returns the number of vertices within a geometry |
| [`ST_OffsetCurve`](#st_offsetcurve) | Returns an offset curve from a linestring |
| [`ST_OrderingEquals`](#st_orderingequals) | Returns true if two geometries are exactly equal (same vertex order) |
| [`ST_Overlaps`](#st_overlaps) | Returns true if the geometries overlap |
| [`ST_Perimeter`](#st_perimeter) | Returns the length of the perimeter of the geometry |
| [`ST_Perimeter_Spheroid`](#st_perimeter_spheroid) | Returns the length of the perimeter in meters using an ellipsoidal model of the earths surface |
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
| [`ST_QuantizeCoordinates`](#st_quantizecoordinates) | Rounds all coordinates to the given number of decimal places |
| [`ST_ReducePrecision`](#st_reduceprecision) | Returns the geometry with all vertices reduced to the given precision |
| [`ST_Relate`](#st_relate) | Returns the DE-9IM intersection matrix string |
| [`ST_RelateMatch`](#st_relatematch) | Tests if a DE-9IM matrix string matches a DE-9IM pattern |
| [`ST_RemovePoint`](#st_removepoint) | Removes a point from a linestring (0-indexed, negative from end) |
| [`ST_RemoveRepeatedPoints`](#st_removerepeatedpoints) | Remove repeated points from a LINESTRING. |
| [`ST_Reverse`](#st_reverse) | Returns the geometry with the order of its vertices reversed |
| [`ST_Scroll`](#st_scroll) | Rotates a closed linestring's start point to the vertex nearest to the given point |
| [`ST_Segmentize`](#st_segmentize) | Densifies a geometry by adding vertices so no segment exceeds max_segment_length |
| [`ST_SetPoint`](#st_setpoint) | Replaces a point in a linestring (0-indexed, negative from end) |
| [`ST_SetSRID`](#st_setsrid) | Sets the SRID of a geometry (no-op in DuckDB — use GEOMETRY('EPSG:XXXX') type for CRS) |
| [`ST_SharedPaths`](#st_sharedpaths) | Returns shared paths between two linear geometries |
| [`ST_ShiftLongitude`](#st_shiftlongitude) | Shifts longitude: negative values get +360, values >180 get -360 |
| [`ST_ShortestLine`](#st_shortestline) | Returns the shortest line between two geometries |
| [`ST_Simplify`](#st_simplify) | Returns a simplified version of the geometry |
| [`ST_SimplifyPolygonHull`](#st_simplifypolygonhull) | Simplifies a polygon while preserving topology |
| [`ST_SimplifyPreserveTopology`](#st_simplifypreservetopology) | Returns a simplified version of the geometry that preserves topology |
| [`ST_SimplifyVW`](#st_simplifyvw) | Simplifies geometry using the Visvalingam-Whyatt area-based algorithm |
| [`ST_Snap`](#st_snap) | Snaps the vertices and segments of a geometry to another geometry's vertices within the given tolerance |
| [`ST_SnapToGrid`](#st_snaptogrid) | Snaps all coordinates to a grid of the given size |
| [`ST_Split`](#st_split) | Splits a geometry by another geometry, returning a geometry collection of the pieces |
| [`ST_SRID`](#st_srid) | Returns the SRID of a geometry (always 0 — DuckDB uses CRS type metadata instead of per-geometry SRIDs) |
| [`ST_StartPoint`](#st_startpoint) | Returns the start point of a LINESTRING. |
| [`ST_Subdivide`](#st_subdivide) | Recursively splits a geometry into sub-geometries until the number of vertices of each are below the threshold given by max_vertices. Accepts any type of input except for a GeometryCollection.Degenerate inputs can lead to results having more than max_vertices vertices due to a recursion depth limit. |
| [`ST_Summary`](#st_summary) | Returns a text summary of a geometry |
| [`ST_SwapOrdinates`](#st_swapordinates) | Swaps two ordinate values in a geometry (e.g., 'xy' swaps x and y) |
| [`ST_SymDifference`](#st_symdifference) | Returns the symmetric difference of two geometries |
| [`ST_TileEnvelope`](#st_tileenvelope) | The `ST_TileEnvelope` scalar function generates tile envelope rectangular polygons from specified zoom level and tile indices. |
| [`ST_Touches`](#st_touches) | Returns true if the geometries touch |
| [`ST_Transform`](#st_transform) | Transforms a geometry between two coordinate systems |
| [`ST_TriangulatePolygon`](#st_triangulatepolygon) | Returns constrained Delaunay triangulation of a polygon |
| [`ST_UnaryUnion`](#st_unaryunion) | Dissolves a geometry collection into a single geometry |
| [`ST_Union`](#st_union) | Returns the union of two geometries |
| [`ST_VoronoiDiagram`](#st_voronoidiagram) | Returns the Voronoi diagram of the supplied MultiPoint geometry |
| [`ST_Within`](#st_within) | Returns true if the first geometry is within the second |
| [`ST_WithinProperly`](#st_withinproperly) | Returns true if the first geometry \"properly\" is contained by the second geometry |
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
| [`ST_Union_Agg`](#st_union_agg) | Computes the union of a set of input geometries |
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
DOUBLE ST_Area (geog GEOGRAPHY)
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
VARCHAR ST_AsText (geog GEOGRAPHY)
```

#### Description

Returns the Well-Known Text (WKT) representation of the geometry

#### Example

```sql
ST_AsText(ST_GeomFromWKB(X'01010000000000000000000000000000000000000000000000'))
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
BLOB ST_AsWKB (geog GEOGRAPHY)
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
GEOGRAPHY ST_Buffer (geog GEOGRAPHY, distance DOUBLE)
GEOGRAPHY ST_Buffer (geog GEOGRAPHY, distance DOUBLE, num_triangles INTEGER)
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

### ST_ConvexHull


#### Signature

```sql
GEOMETRY ST_ConvexHull (geom GEOMETRY)
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
DOUBLE ST_Distance (geog1 GEOGRAPHY, geog2 GEOGRAPHY)
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

### ST_DWithin


#### Signatures

```sql
BOOLEAN ST_DWithin (geom1 GEOMETRY, geom2 GEOMETRY, distance DOUBLE)
BOOLEAN ST_DWithin (geog1 GEOGRAPHY, geog2 GEOGRAPHY, distance DOUBLE)
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


#### Signature

```sql
GEOMETRY ST_Envelope (geom GEOMETRY)
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

### ST_GeogFromText


#### Signature

```sql
GEOGRAPHY ST_GeogFromText (wkt VARCHAR)
```

#### Description

Creates a GEOGRAPHY from its WKT representation.

The coordinates are longitude and latitude in degrees on WGS84, in that order. Longitudes outside of [-180, 180] and latitudes outside of [-90, 90] raise an error. `ST_GeogFromText`, `ST_GeogFromWKT` and `ST_GeographyFromText` are the same function.

#### Example

```sql
SELECT ST_GeogFromText('POINT(4.3517 50.8503)');
```

----

### ST_GeogFromWKB


#### Signature

```sql
GEOGRAPHY ST_GeogFromWKB (wkb BLOB)
```

#### Description

Creates a GEOGRAPHY from its WKB representation.

The coordinates are longitude and latitude in degrees on WGS84, in that order. Longitudes outside of [-180, 180] and latitudes outside of [-90, 90] raise an error.

#### Example

```sql
SELECT ST_GeogFromWKB(ST_AsWKB(ST_Point(4.3517, 50.8503)));
```

----

### ST_GeogFromWKT


#### Signature

```sql
GEOGRAPHY ST_GeogFromWKT (wkt VARCHAR)
```

#### Description

Creates a GEOGRAPHY from its WKT representation.

The coordinates are longitude and latitude in degrees on WGS84, in that order. Longitudes outside of [-180, 180] and latitudes outside of [-90, 90] raise an error. `ST_GeogFromText`, `ST_GeogFromWKT` and `ST_GeographyFromText` are the same function.

#### Example

```sql
SELECT ST_GeogFromText('POINT(4.3517 50.8503)');
```

----

### ST_GeogPoint


#### Signature

```sql
GEOGRAPHY ST_GeogPoint (longitude DOUBLE, latitude DOUBLE)
```

#### Description

Creates a GEOGRAPHY point from a longitude and a latitude in degrees on WGS84

#### Example

```sql
SELECT ST_GeogPoint(4.3517, 50.8503);
```

----

### ST_GeographyFromText


#### Signature

```sql
GEOGRAPHY ST_GeographyFromText (wkt VARCHAR)
```

#### Description

Creates a GEOGRAPHY from its WKT representation.

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

Returns the Nth geometry from a geometry collection (0-indexed)

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

### ST_Intersection


#### Signature

```sql
GEOMETRY ST_Intersection (geom1 GEOMETRY, geom2 GEOMETRY)
```

#### Description

Returns the intersection of two geometries

----

### ST_Intersects


#### Signatures

```sql
BOOLEAN ST_Intersects (box1 BOX_2D, box2 BOX_2D)
BOOLEAN ST_Intersects (geom1 GEOMETRY, geom2 GEOMETRY)
BOOLEAN ST_Intersects (geog1 GEOGRAPHY, geog2 GEOGRAPHY)
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
DOUBLE ST_Length (geog GEOGRAPHY)
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


#### Signature

```sql
BOOLEAN ST_Overlaps (geom1 GEOMETRY, geom2 GEOMETRY)
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
DOUBLE ST_Perimeter (geog GEOGRAPHY)
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


#### Signature

```sql
GEOMETRY ST_Polygon (line GEOMETRY)
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
GEOGRAPHY ST_Project (origin GEOGRAPHY, distance DOUBLE, azimuth DOUBLE)
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

### ST_Reverse


#### Signature

```sql
GEOMETRY ST_Reverse (geom GEOMETRY)
```

#### Description

Returns the geometry with the order of its vertices reversed

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
GEOGRAPHY ST_Segmentize (geog GEOGRAPHY, max_segment_length DOUBLE)
```

#### Description

Densifies a geometry by adding vertices so no segment exceeds max_segment_length

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

### ST_SetSRID


#### Signature

```sql
GEOMETRY ST_SetSRID (geom GEOMETRY, srid INTEGER)
```

#### Description

Sets the SRID of a geometry (no-op in DuckDB — use GEOMETRY('EPSG:XXXX') type for CRS)

#### Example

```sql
SELECT ST_SetSRID(ST_Point(1, 2), 4326)
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

### ST_Snap


#### Signature

```sql
GEOMETRY ST_Snap (geom GEOMETRY, snap_to GEOMETRY, tolerance DOUBLE)
```

#### Description

Snaps the vertices and segments of a geometry to another geometry's vertices within the given tolerance

----

### ST_SnapToGrid


#### Signature

```sql
GEOMETRY ST_SnapToGrid (geom GEOMETRY, size DOUBLE)
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


#### Signature

```sql
INTEGER ST_SRID (geom GEOMETRY)
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


#### Signature

```sql
BOOLEAN ST_Touches (geom1 GEOMETRY, geom2 GEOMETRY)
```

#### Description

Returns true if the geometries touch

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

### ST_VoronoiDiagram


#### Signature

```sql
GEOMETRY ST_VoronoiDiagram (geom GEOMETRY)
```

#### Description

Returns the Voronoi diagram of the supplied MultiPoint geometry

----

### ST_Within


#### Signatures

```sql
BOOLEAN ST_Within (geom1 POINT_2D, geom2 POLYGON_2D)
BOOLEAN ST_Within (geom1 GEOMETRY, geom2 GEOMETRY)
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

Note: OVER (PARTITION BY ...) currently requires an ORDER BY clause
(e.g. OVER (PARTITION BY grp ORDER BY id)). Without an ORDER BY,
DuckDB's window_self_join optimizer rewrites the query into a grouped
aggregate, which this function cannot satisfy. The ORDER BY expression
does not affect clustering results — the whole partition is always
used — it only disables the rewrite.

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

Note: OVER (PARTITION BY ...) requires an ORDER BY clause — see
ST_ClusterDBSCAN for the rationale.

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

### ST_Union_Agg


#### Signature

```sql
GEOMETRY ST_Union_Agg (col0 GEOMETRY)
```

#### Description

Computes the union of a set of input geometries

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

