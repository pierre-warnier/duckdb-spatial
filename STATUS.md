# Status

## Where the project stands
- Working branch `v2.0-cyanoptera` (and `main`, which follows it), built against DuckDB v2.0.0 (the `duckdb/` submodule is the
  v2.0.0 release tag, `d54cbbe3df`). `v1.5-variegata` (DuckDB v1.5.6) is maintenance only, for the 1.5.6 Windows binary.
- CI: `.github/workflows/MainDistributionPipeline.yml` builds every target with DuckDB's extension-ci-tools on GitHub-hosted runners,
  except `linux_amd64_musl` and `windows_amd64_mingw` (wrong ABI for the official CLI).

## Windows x64
- `windows_amd64` is built with MSVC. `M_PI` is not ISO C++ and MSVC only defines it with `_USE_MATH_DEFINES`: use `MathUtil::PI`
  (`src/spatial/util/math.hpp`) instead.
- Portable C++ rules learnt from MSVC/macOS/wasm CI: this line is C++17 (v1.5-variegata is C++11: no structured bindings
  there, keep shared code C++11 so fixes cherry-pick both ways), include every std header you use (`<numeric>` for `std::iota`: MSVC's STL
  does not pull it in transitively), and give `MinValue`/`MaxValue` one explicit type (`idx_t` is not `size_t` on macOS/wasm).
- The `windows-smoke` job loads the build in the official v2.0.0 CLI (`duckdb-cli-windows-amd64.tar.gz`, SHA-256 pinned) on `windows-latest` and runs `test/smoke/startup.sql`
  (version, `LOAD`, `ST_ReadOSM` on `test/data/osm/tiny.osm.pbf`, `pgr_dijkstra` on a graph built from it and on the pgRouting sample graph).
- On `main` and `v2.0-cyanoptera`, `windows-release` publishes `spatial.duckdb_extension` and its `.sha256` as a release tagged
  `<duckdb version>-fork.<n>` (e.g. `v2.0.0-fork.3`; the title names the fork commit). Releases are only made by the workflow, and an existing
  release is never overwritten.
- The official Windows CLI links the C runtime statically (it imports only KERNEL32, WS2_32 and RstrtMgr).

## Decisions
- The upstream deploy job was removed: it uploads to DuckDB's S3 bucket with secrets that only exist in the duckdb organisation.
- The architecture notes live in `docs/architecture.md`.

## Next
- Rebuild and re-release on every DuckDB patch release (an extension only loads in the exact DuckDB version it was built for).
