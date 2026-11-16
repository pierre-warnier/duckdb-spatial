# Status

## Where the project stands
- Working branch `v1.5-variegata`, built against DuckDB v1.5.6 (the `duckdb/` submodule is the v1.5.6 release tag, `069cc9f9b5`).
- `v2.0-cyanoptera` carries the port to DuckDB v2.0.0.
- CI: `.github/workflows/MainDistributionPipeline.yml` builds every target with DuckDB's extension-ci-tools on GitHub-hosted runners,
  except `linux_amd64_musl` and `windows_amd64_mingw` (wrong ABI for the official CLI).

## Windows x64
- `windows_amd64` is built with MSVC. `M_PI` is not ISO C++ and MSVC only defines it with `_USE_MATH_DEFINES`: use `MathUtil::PI`
  (`src/spatial/util/math.hpp`) instead.
- The `windows-smoke` job loads the build in the official v1.5.6 CLI on `windows-latest` and runs `test/smoke/startup.sql`
  (version, `LOAD`, `ST_ReadOSM` on `test/data/osm/tiny.osm.pbf`, `pgr_dijkstra` on a graph built from it and on the pgRouting sample graph).
- On the default branch, `windows-release` publishes `spatial.duckdb_extension` and its `.sha256` as a release tagged
  `spatial-windows_amd64-duckdb-<duckdb version>-<commit>`. Releases are only made by the workflow.
- The official Windows CLI links the C runtime statically (it imports only KERNEL32, WS2_32 and RstrtMgr).

## Decisions
- The upstream deploy job was removed: it uploads to DuckDB's S3 bucket with secrets that only exist in the duckdb organisation.
- The architecture notes live in `docs/architecture.md`.

## Next
- Apply the same Windows fixes and jobs to `v2.0-cyanoptera` once its DuckDB version is chosen for Windows users.
