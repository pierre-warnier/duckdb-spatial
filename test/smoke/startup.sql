-- Start-up smoke test for a locally built spatial extension (used by the Windows release job):
-- LOAD, ST_ReadOSM, pgr_dijkstra, pgr_drivingDistance and ST_KNN.
-- Run from the repository root, with the extension copied there:
--   EXPECTED_DUCKDB_VERSION=v1.5.6 duckdb -unsigned -bail -f test/smoke/startup.sql
-- Every check raises an error on mismatch, so the CLI exits non-zero.
.bail on
LOAD './spatial.duckdb_extension';

SET geometry_always_xy = true;

-- 0. The CLI is the pinned release (EXPECTED_DUCKDB_VERSION, e.g. v1.5.6) and the extension loads.
SELECT CASE WHEN version() = getenv('EXPECTED_DUCKDB_VERSION') THEN 'duckdb ' || version()
            ELSE error('DuckDB CLI is ' || version() || ', expected ' || getenv('EXPECTED_DUCKDB_VERSION')) END AS check_version;
SELECT CASE WHEN count(*) = 1 THEN 'spatial loaded: ' || any_value(extension_version)
            ELSE error('spatial extension is not loaded') END AS check_load
FROM duckdb_extensions() WHERE extension_name = 'spatial' AND loaded;

-- 1. ST_ReadOSM on the committed fixture (5 nodes, 3 ways, 1 relation).
CREATE TABLE osm AS SELECT * FROM ST_ReadOSM('test/data/osm/tiny.osm.pbf');
SELECT CASE WHEN (SELECT string_agg(kind || '=' || n, ',' ORDER BY kind)
                  FROM (SELECT kind::VARCHAR AS kind, count(*) AS n FROM osm GROUP BY ALL))
                 = 'node=5,relation=1,way=3'
            THEN 'ST_ReadOSM ok' ELSE error('ST_ReadOSM: unexpected element counts') END AS check_osm;
SELECT CASE WHEN tags['name'] = 'Fixture Cafe' AND abs(lat - 50.86) < 1e-7 AND abs(lon - 4.37) < 1e-7
            THEN 'ST_ReadOSM tags/coords ok' ELSE error('ST_ReadOSM: node 5 tags or coordinates wrong') END AS check_osm_node
FROM osm WHERE kind = 'node' AND id = 5;

-- 2. pgr_dijkstra on a road graph built from the fixture: way segments become edges, oneway=yes forbids reverse travel.
CREATE TABLE nodes AS SELECT id, lat, lon FROM osm WHERE kind = 'node';
CREATE TABLE edges AS
WITH seg AS (
    SELECT w.id AS way_id, w.tags['oneway'] = 'yes' AS oneway, i,
           w.refs[i] AS source, w.refs[i + 1] AS target
    FROM osm w, range(1, len(w.refs)) r(i)
    WHERE w.kind = 'way' AND w.tags['highway'] IS NOT NULL
)
SELECT row_number() OVER (ORDER BY way_id, i) AS id, s.source, s.target,
       ST_Distance_Sphere(ST_Point(a.lon, a.lat), ST_Point(b.lon, b.lat)) AS cost,
       CASE WHEN coalesce(s.oneway, false) THEN -1
            ELSE ST_Distance_Sphere(ST_Point(a.lon, a.lat), ST_Point(b.lon, b.lat)) END AS reverse_cost
FROM seg s JOIN nodes a ON a.id = s.source JOIN nodes b ON b.id = s.target;
-- From node 5 to node 1 the oneway 3->5 cannot be used backwards, so the path must be 5-4-2-1.
SELECT CASE WHEN list(node ORDER BY path_seq) = [5, 4, 2, 1]
             AND abs(max(agg_cost) - (SELECT sum(cost) FROM edges WHERE (source, target) IN ((1, 2), (2, 4), (4, 5)))) < 1e-6
            THEN 'pgr_dijkstra on OSM graph ok' ELSE error('pgr_dijkstra on OSM graph: unexpected path') END AS check_route
FROM pgr_dijkstra((SELECT id, source, target, cost, reverse_cost FROM edges), 5, 1);

-- 3. pgr_dijkstra on the pgRouting sample graph, expected output from the pgRouting documentation.
CREATE TABLE sample AS SELECT * FROM (VALUES
    (1, 5, 6, 1, 1), (2, 6, 10, -1, 1), (3, 10, 15, -1, 1), (4, 6, 7, 1, 1), (5, 10, 11, 1, -1), (6, 1, 3, 1, 1),
    (7, 3, 7, 1, 1), (8, 7, 11, 1, 1), (9, 11, 16, 1, 1), (10, 7, 8, 1, 1), (11, 11, 12, 1, -1), (12, 8, 12, 1, -1),
    (13, 12, 17, 1, -1), (14, 8, 9, 1, 1), (15, 16, 17, 1, 1), (16, 15, 16, 1, 1), (17, 2, 4, 1, 1), (18, 13, 14, 1, 1)
) t(id, source, target, cost, reverse_cost);
SELECT CASE WHEN list([node, edge] ORDER BY path_seq) = [[6, 4], [7, 8], [11, 9], [16, 16], [15, 3], [10, -1]]
             AND max(agg_cost) = 5.0
            THEN 'pgr_dijkstra sample graph ok' ELSE error('pgr_dijkstra sample graph: unexpected path') END AS check_sample
FROM pgr_dijkstra((SELECT id, source, target, cost, reverse_cost FROM sample), 6, 10);

-- 4. pgr_drivingDistance on the same sample graph: every node within cost 3 of node 11 (pgRouting documentation).
SELECT CASE WHEN list(node ORDER BY node) = [1, 3, 5, 6, 7, 8, 9, 10, 11, 12, 15, 16, 17] AND max(agg_cost) = 3.0
            THEN 'pgr_drivingDistance ok' ELSE error('pgr_drivingDistance: unexpected reachable set') END AS check_driving_distance
FROM pgr_drivingDistance((SELECT id, source, target, cost, reverse_cost FROM sample), 11, 3.0);

-- 5. ST_KNN join: the 2 nearest build points of each probe point, against a brute-force answer and a fixed one.
CREATE TABLE knn_probe AS SELECT * FROM (VALUES (1, ST_Point(0, 0)), (2, ST_Point(10, 0))) t(pid, geom);
CREATE TABLE knn_build AS SELECT * FROM (VALUES
    (1, ST_Point(1, 0)), (2, ST_Point(0, 2)), (3, ST_Point(3, 0)), (4, ST_Point(0, 4)), (5, ST_Point(9, 0)), (6, ST_Point(12.5, 0))
) t(bid, geom);
CREATE TABLE knn AS SELECT p.pid, b.bid FROM knn_probe p JOIN knn_build b ON ST_KNN(p.geom, b.geom, 2);
SELECT CASE WHEN (SELECT list([pid, bid] ORDER BY pid, bid) FROM knn) = [[1, 1], [1, 2], [2, 5], [2, 6]]
             AND (SELECT list([pid, bid] ORDER BY pid, bid) FROM knn)
               = (SELECT list([pid, bid] ORDER BY pid, bid) FROM (
                    SELECT p.pid, b.bid, row_number() OVER (PARTITION BY p.pid ORDER BY ST_Distance(p.geom, b.geom)) AS r
                    FROM knn_probe p, knn_build b) WHERE r <= 2)
            THEN 'ST_KNN ok' ELSE error('ST_KNN: unexpected nearest neighbours') END AS check_knn;

SELECT 'ALL CHECKS PASSED' AS result;
