"""Benchmark of this extension against PostGIS / pgRouting. See docs/benchmarks.md."""
import duckdb, time, sys, json, os, subprocess

HERE = os.path.dirname(os.path.abspath(__file__))
EXT = os.environ.get('SPATIAL_EXTENSION', 'build/release/extension/spatial/spatial.duckdb_extension')
DB = os.environ.get('BENCH_DB', 'bench.db')
PGENV = dict(os.environ, PGPASSWORD=os.environ.get('BENCH_PGPASSWORD', 'bench'))
PGHOST = os.environ.get('BENCH_PGHOST', '127.0.0.1')
PGPORT = os.environ.get('BENCH_PGPORT', '55432')
PSQL = ['psql', '-h', PGHOST, '-p', PGPORT, '-U', 'postgres', '-At', '-F', '|', '-v', 'ON_ERROR_STOP=1']
RUNS = int(os.environ.get('RUNS', '2'))
ONLY = os.environ.get('ONLY', '')
SMALL = os.environ.get('SMALL', '') == '1'

con = duckdb.connect(DB, config={'allow_unsigned_extensions': 'true'})
con.execute(f"LOAD '{EXT}'")

def pg(sql, workers=8, timeout='1800s'):
    pre = (f"SET statement_timeout = '{timeout}'; SET max_parallel_workers_per_gather = {workers}; "
           "SET parallel_setup_cost = 0; SET parallel_tuple_cost = 0.001; SET min_parallel_table_scan_size = 0; "
           "SET postgis.gdal_enabled_drivers = 'GTiff'; SET client_min_messages = warning;")
    out = subprocess.run(PSQL + ['-c', pre, '-c', sql], env=PGENV, capture_output=True, text=True)
    if out.returncode != 0:
        raise RuntimeError(out.stderr.strip()[:400])
    lines = [l for l in out.stdout.strip().split('\n') if l and l != 'SET']
    return [tuple(lines[-1].split('|'))] if lines else []

def duck(sql, threads):
    con.execute(f"SET threads = {threads}")
    return con.execute(sql).fetchall()

def timed(fn):
    best = None; result = None
    for _ in range(RUNS):
        t0 = time.perf_counter(); r = fn(); dt = time.perf_counter() - t0
        if best is None or dt < best:
            best = dt; result = r
    return best, result

def num(v):
    try:
        return float(v)
    except Exception:
        return v

def close(rel):
    def f(a, b):
        if len(a[0]) != len(b[0]):
            return False
        for x, y in zip(a[0], b[0]):
            x, y = num(x), num(y)
            if isinstance(x, float) and isinstance(y, float):
                if abs(x - y) > rel * max(1.0, abs(x), abs(y)):
                    return False
            elif str(x) != str(y):
                return False
        return True
    return f

results = []
def report(name, duck_sql, pg_sql, check=close(1e-9), pg_parallel=True, duck_threads=(1, 8, 32)):
    if ONLY and ONLY not in name:
        return
    row = {'bench': name}
    try:
        for threads in duck_threads:
            row[f'duckdb_{threads}'], r_duck = timed(lambda: duck(duck_sql, threads))
        row['duckdb_result'] = [str(v) for v in r_duck[0]]
    except Exception as e:
        row['duckdb_error'] = str(e)[:300]; r_duck = None
    try:
        row['postgis_1'], r_pg = timed(lambda: pg(pg_sql, 0))
        if pg_parallel:
            row['postgis_8'], r_pg = timed(lambda: pg(pg_sql, 8))
        row['postgis_result'] = [str(v) for v in r_pg[0]]
    except Exception as e:
        row['postgis_error'] = str(e)[:300]; r_pg = None
    row['same'] = bool(r_duck and r_pg and check(r_duck, r_pg))
    results.append(row)
    print(json.dumps(row), flush=True)

E_D = "NOT ST_IsEmpty(geom)"
E_P = "NOT ST_IsEmpty(geometry)"
S7 = "%777" if SMALL else "%7"
S77 = "%7777" if SMALL else "%77"

# ---------------------------------------------------------------------------------------------------------------------
# Vector
# ---------------------------------------------------------------------------------------------------------------------
report('transform_points',
    f"SELECT round(sum(ST_X(g)), 3), round(sum(ST_Y(g)), 3) FROM (SELECT ST_Transform(geom, 'EPSG:4326', 'EPSG:3812', always_xy := true) g FROM padr WHERE {E_D} AND padr_code LIKE '{'%7' if SMALL else '%'}')",
    f"SELECT round(sum(ST_X(g))::numeric, 3), round(sum(ST_Y(g))::numeric, 3) FROM (SELECT ST_Transform(ST_SetSRID(geometry, 4326), 3812) g FROM geoadmin.padr WHERE {E_P} AND padr_code LIKE '{'%7' if SMALL else '%'}') t")

report('point_in_polygon_join',
    f"SELECT count(*), count(DISTINCT n.nis6_code) FROM (SELECT * FROM padr WHERE padr_code LIKE '{'%7' if SMALL else '%'}') p JOIN nis6 n ON ST_Intersects(n.geom, p.geom)",
    f"SELECT count(*), count(DISTINCT n.nis6_code) FROM geoadmin.padr p JOIN geoadmin.nis6 n ON ST_Intersects(n.geometry, p.geometry) WHERE p.padr_code LIKE '{'%7' if SMALL else '%'}'")

for k in (1, 5):
    report(f'knn_k{k}',
        f"SELECT count(*), round(sum(ST_Distance(p.geom, s.geom)), 6) FROM (SELECT * FROM padr WHERE padr_code LIKE '{S7}' AND {E_D}) p JOIN ss s ON ST_KNN(p.geom, s.geom, {k})",
        f"SELECT count(*), round(sum(d)::numeric, 6) FROM (SELECT s.d FROM geoadmin.padr p CROSS JOIN LATERAL (SELECT s.geometry <-> p.geometry AS d FROM geoadmin.ss s ORDER BY s.geometry <-> p.geometry LIMIT {k}) s WHERE p.padr_code LIKE '{S7}' AND NOT ST_IsEmpty(p.geometry)) t")

report('geography_distance_point_pairs',
    f"SELECT count(*), round(sum(ST_Distance(p.geom::GEOG, s.geom::GEOG)), 3) FROM (SELECT * FROM padr WHERE padr_code LIKE '{'%7' if SMALL else '%'}') p JOIN ss s ON p.ss_code = s.ss_code",
    f"SELECT count(*), round(sum(ST_Distance(ST_SetSRID(p.geometry, 4326)::geography, ST_SetSRID(s.geometry, 4326)::geography))::numeric, 3) FROM geoadmin.padr p JOIN geoadmin.ss s ON p.ss_code = s.ss_code WHERE p.padr_code LIKE '{'%7' if SMALL else '%'}'")

report('geography_distance_points_to_9680_vertex_line',
    f"SELECT count(*), round(sum(ST_Distance(p.geom::GEOG, m.geom::GEOG)), 3) FROM (SELECT geom FROM padr WHERE padr_code LIKE '{S77}' AND {E_D}) p, (SELECT ST_ExteriorRing(ST_GeometryN(geom, 1)) geom FROM muni WHERE muni_code = '71072') m",
    f"SELECT count(*), round(sum(ST_Distance(ST_SetSRID(p.geometry, 4326)::geography, m.geom::geography))::numeric, 3) FROM (SELECT geometry FROM geoadmin.padr WHERE padr_code LIKE '{S77}' AND {E_P}) p, (SELECT ST_SetSRID(ST_ExteriorRing(ST_GeometryN(geometry, 1)), 4326) geom FROM geoadmin.muni WHERE muni_code = '71072') m",
    check=close(1e-6))

report('dbscan',
    f"SELECT count(DISTINCT c), count(*) FILTER (WHERE c IS NULL) FROM (SELECT ST_ClusterDBSCAN(geom, 0.001, 5) OVER () c FROM padr WHERE padr_code LIKE '{S7}' AND {E_D})",
    f"SELECT count(DISTINCT c), count(*) FILTER (WHERE c IS NULL) FROM (SELECT ST_ClusterDBSCAN(geometry, 0.001, 5) OVER () c FROM geoadmin.padr WHERE padr_code LIKE '{S7}' AND {E_P}) t")

report('buffer_union',
    f"SELECT round(ST_Area(ST_Union_Agg(ST_Buffer(geom, 0.001))), 9) FROM ss WHERE ss_code LIKE '{S7}'",
    f"SELECT round(ST_Area(ST_Union(ST_Buffer(geometry, 0.001)))::numeric, 9) FROM geoadmin.ss WHERE ss_code LIKE '{S7}'",
    check=close(1e-6))

report('polygon_overlay',
    "SELECT count(*), round(sum(ST_Area(ST_Intersection(n.geom, m.geom))), 9) FROM nis6 n JOIN muni m ON ST_Intersects(n.geom, m.geom)",
    "SELECT count(*), round(sum(ST_Area(ST_Intersection(n.geometry, m.geometry)))::numeric, 9) FROM geoadmin.nis6 n JOIN geoadmin.muni m ON ST_Intersects(n.geometry, m.geometry)",
    check=close(1e-6))

# ---------------------------------------------------------------------------------------------------------------------
# Routing: grid graph, identical edges on both sides
# ---------------------------------------------------------------------------------------------------------------------
N = 200 if SMALL else 700
LAST = N * (N + 1) + N
if not ONLY or 'routing' in ONLY:
    con.execute("SELECT setseed(0.42)")
    con.execute(f"""CREATE OR REPLACE TABLE edges AS
        SELECT r * {N + 1} + c + 1 AS id, r * {N + 1} + c AS "source", r * {N + 1} + c + 1 AS "target",
               round(1 + random(), 6) AS cost, round(1 + random(), 6) AS reverse_cost
        FROM range({N + 1}) a(r), range({N}) b(c)
        UNION ALL
        SELECT 10000000 + r * {N + 1} + c + 1, r * {N + 1} + c, (r + 1) * {N + 1} + c, round(1 + random(), 6), round(1 + random(), 6)
        FROM range({N}) a(r), range({N + 1}) b(c)""")
    con.execute("LOAD postgres")
    con.execute(f"ATTACH 'host={PGHOST} port={PGPORT} user=postgres password={PGENV['PGPASSWORD']} dbname=postgres' AS b (TYPE postgres)")
    con.execute("CALL postgres_execute('b', 'drop table if exists public.edges')")
    con.execute('CREATE TABLE b.public.edges AS SELECT id::BIGINT id, "source"::BIGINT "source", "target"::BIGINT "target", cost, reverse_cost FROM edges')
    con.execute("DETACH b")
    pg("create index on edges (id); analyze edges")

ESQL = "select id, source, target, cost, reverse_cost from edges"
report('routing_dijkstra_one_to_one',
    f"SELECT count(*), round(max(agg_cost), 6) FROM pgr_dijkstra((SELECT * FROM edges), 0, {LAST})",
    f"SELECT count(*), round(max(agg_cost)::numeric, 6) FROM pgr_dijkstra('{ESQL}', 0, {LAST})", pg_parallel=False)
report('routing_dijkstra_cost_20x20',
    f"SELECT count(*), round(sum(agg_cost), 6) FROM pgr_dijkstraCost((SELECT * FROM edges), [i * {LAST // 20} FOR i IN range(20)], [i * {LAST // 20} + {N // 2} FOR i IN range(20)])",
    f"SELECT count(*), round(sum(agg_cost)::numeric, 6) FROM pgr_dijkstraCost('{ESQL}', (select array_agg(i * {LAST // 20}) from generate_series(0, 19) i), (select array_agg(i * {LAST // 20} + {N // 2}) from generate_series(0, 19) i))",
    pg_parallel=False)
report('routing_driving_distance',
    f"SELECT count(*), round(sum(agg_cost), 4) FROM pgr_drivingDistance((SELECT * FROM edges), {LAST // 2}, 300)",
    f"SELECT count(*), round(sum(agg_cost)::numeric, 4) FROM pgr_drivingDistance('{ESQL}', {LAST // 2}, 300)", pg_parallel=False, check=close(1e-9))
report('routing_connected_components',
    "SELECT count(*), count(DISTINCT component) FROM pgr_connectedComponents((SELECT * FROM edges))",
    f"SELECT count(*), count(DISTINCT component) FROM pgr_connectedComponents('{ESQL}')", pg_parallel=False)
report('routing_ksp_k5',
    f"SELECT count(DISTINCT path_id), round(max(agg_cost), 6) FROM pgr_KSP((SELECT * FROM edges), 0, {N * (N + 1) // 4 + N // 4}, 5)",
    f"SELECT count(DISTINCT path_id), round(max(agg_cost)::numeric, 6) FROM pgr_KSP('{ESQL}', 0, {N * (N + 1) // 4 + N // 4}, 5)", pg_parallel=False)

# ---------------------------------------------------------------------------------------------------------------------
# Raster: one analytic DEM, cut into 256x256 tiles, identical bytes on both sides
# ---------------------------------------------------------------------------------------------------------------------
W = 1024 if SMALL else 4096
if not ONLY or 'raster' in ONLY:
    con.execute(f"""CREATE OR REPLACE TABLE dem AS SELECT unnest(ST_Tile(ST_MapAlgebra(
        ST_AddBand(ST_MakeEmptyRaster({W}, {W}, 150000, 170000, 10, -10, 0, 0, 31370), '32BF', 0, -9999), 1, '32BF',
        '100 + 50 * sin([rast.x] / 97.0) + 30 * cos([rast.y] / 61.0) + [rast.x] * 0.01'), 256, 256)) AS rast""")
    con.execute("CREATE OR REPLACE TABLE dem AS SELECT row_number() OVER () AS rid, rast FROM dem")
    con.execute("LOAD postgres")
    con.execute(f"ATTACH 'host={PGHOST} port={PGPORT} user=postgres password={PGENV['PGPASSWORD']} dbname=postgres' AS b (TYPE postgres)")
    con.execute("CALL postgres_execute('b', 'drop table if exists public.dem; drop table if exists public.dem_tif')")
    con.execute("CREATE TABLE b.public.dem_tif AS SELECT rid, ST_AsTIFF(rast)::BLOB AS tif FROM dem")
    con.execute("DETACH b")
    pg("create table dem as select rid, ST_FromGDALRaster(tif, 31370) rast from dem_tif; create index on dem using gist (ST_ConvexHull(rast)); analyze dem")
    con.execute("SELECT setseed(0.7)")
    con.execute(f"CREATE OR REPLACE TABLE rpts AS SELECT i AS id, 150000 + random() * {W * 10} AS x, 170000 - random() * {W * 10} AS y FROM range({20000 if SMALL else 200000}) t(i)")
    con.execute("LOAD postgres")
    con.execute(f"ATTACH 'host={PGHOST} port={PGPORT} user=postgres password={PGENV['PGPASSWORD']} dbname=postgres' AS b (TYPE postgres)")
    con.execute("CALL postgres_execute('b', 'drop table if exists public.rpts')")
    con.execute("CREATE TABLE b.public.rpts AS SELECT * FROM rpts")
    con.execute("DETACH b")
    pg("alter table rpts add column geom geometry; update rpts set geom = ST_SetSRID(ST_MakePoint(x, y), 31370); create index on rpts using gist (geom); analyze rpts")

report('raster_summarystats_tiles',
    "SELECT count(*), sum(s.count), round(sum(s.sum), 2), round(min(s.min), 4), round(max(s.max), 4) FROM (SELECT ST_SummaryStats(rast) s FROM dem)",
    "SELECT count(*), sum((s).count), round(sum((s).sum)::numeric, 2), round(min((s).min)::numeric, 4), round(max((s).max)::numeric, 4) FROM (SELECT ST_SummaryStats(rast) s FROM dem) t",
    check=close(1e-6))
report('raster_mapalgebra_tiles',
    "SELECT count(*), round(sum((ST_SummaryStats(r)).sum), 1) FROM (SELECT ST_MapAlgebra(rast, 1, '32BF', '[rast] * 2 + 1') r FROM dem)",
    "SELECT count(*), round(sum((ST_SummaryStats(r)).sum)::numeric, 1) FROM (SELECT ST_MapAlgebra(rast, 1, '32BF', '[rast] * 2 + 1') r FROM dem) t",
    check=close(1e-6))
report('raster_slope_tiles',
    "SELECT count(*), round(avg((ST_SummaryStats(r)).mean), 3) FROM (SELECT ST_Slope(rast, 1, '32BF') r FROM dem)",
    "SELECT count(*), round(avg((ST_SummaryStats(r)).mean)::numeric, 3) FROM (SELECT ST_Slope(rast, 1, '32BF') r FROM dem) t",
    check=close(2e-2))
report('raster_value_at_points',
    "SELECT count(*), count(v), round(sum(v), 2) FROM (SELECT ST_Value(d.rast, ST_Point(p.x, p.y)) v FROM rpts p JOIN dem d ON ST_Intersects(ST_Envelope(d.rast), ST_Point(p.x, p.y)))",
    "SELECT count(*), count(v), round(sum(v)::numeric, 2) FROM (SELECT ST_Value(d.rast, p.geom) v FROM rpts p JOIN dem d ON ST_Intersects(ST_ConvexHull(d.rast), p.geom)) t",
    check=close(1e-3))
report('raster_union_tiles',
    "SELECT ST_Width(r), ST_Height(r), round((ST_SummaryStats(r)).sum, 1) FROM (SELECT ST_Union_Agg(rast) r FROM dem)",
    "SELECT ST_Width(r), ST_Height(r), round((ST_SummaryStats(r)).sum::numeric, 1) FROM (SELECT ST_Union(rast) r FROM dem) t",
    check=close(1e-6), pg_parallel=False)
report('raster_polygonize_reclass',
    "SELECT count(*), round(sum(ST_Area(p.geom)), 1) FROM (SELECT unnest(ST_DumpAsPolygons(ST_Reclass(rast, 1, '[0-100):1, [100-150):2, [150-400]:3', '8BUI', 0))) p FROM dem WHERE rid <= 16)",
    "SELECT count(*), round(sum(ST_Area((p).geom))::numeric, 1) FROM (SELECT ST_DumpAsPolygons(ST_Reclass(rast, 1, '[0-100):1, [100-150):2, [150-400]:3', '8BUI', 0)) p FROM dem WHERE rid <= 16) t",
    check=close(1e-6))

# ---------------------------------------------------------------------------------------------------------------------
# Topology
# ---------------------------------------------------------------------------------------------------------------------
TOPO = "nis6_code LIKE '2400%'" if SMALL else "nis6_code LIKE '24%'"
def duck_topo(threads=32):
    con.execute(f"SET threads = {threads}")
    try:
        con.execute("CALL DropTopology('bt')")
    except Exception:
        pass
    con.execute("CALL CreateTopology('bt', 0)")
    con.execute(f"SET VARIABLE lw = (SELECT ST_Collect(list(geom)) FROM nis6 WHERE {TOPO})")
    con.execute("SELECT * FROM ST_CreateTopoGeo('bt', getvariable('lw'))").fetchall()
    return con.execute("SELECT (SELECT count(*) FROM bt.node), (SELECT count(*) FROM bt.edge_data), (SELECT count(*) FROM bt.face WHERE face_id <> 0)").fetchall()

def pg_topo():
    return pg(f"""select count(*) from (select topology.DropTopology(name) from topology.topology where name = 'bt') t;
        select topology.CreateTopology('bt', 0) > 0;
        select length(topology.ST_CreateTopoGeo('bt', (select ST_Collect(geometry) from geoadmin.nis6 where {TOPO}))) > 0;
        select (select count(*) from bt.node), (select count(*) from bt.edge_data), (select count(*) from bt.face where face_id <> 0)""", timeout='3600s')

if not ONLY or 'topology' in ONLY:
    row = {'bench': 'topology_create_topo_geo', 'polygons': con.execute(f"SELECT count(*), sum(ST_NPoints(geom)) FROM nis6 WHERE {TOPO}").fetchall()[0]}
    try:
        row['duckdb_1'], r = timed(lambda: duck_topo(1))
        row['duckdb_32'], r = timed(duck_topo); row['duckdb_result'] = [str(v) for v in r[0]]
    except Exception as e:
        row['duckdb_error'] = str(e)[:300]
    try:
        t0 = time.perf_counter(); r = pg_topo(); row['postgis_1'] = time.perf_counter() - t0; row['postgis_result'] = list(r[0])
    except Exception as e:
        row['postgis_error'] = str(e)[:300]
    row['same'] = row.get('duckdb_result') == row.get('postgis_result')
    results.append(row); print(json.dumps(row), flush=True)

    report('topology_validate',
        "SELECT count(*) FROM ValidateTopology('bt')",
        "SELECT count(*) FROM topology.ValidateTopology('bt')", pg_parallel=False, duck_threads=(1, 32), check=lambda a, b: True)

json.dump(results, open(os.path.join(HERE, 'bench_results.json' if not ONLY else f'bench_results_{ONLY}.json'), 'w'), indent=1)
