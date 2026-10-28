#!/usr/bin/env bash
# Rebuild the extension for a DuckDB release, e.g. after a patch release:
#
#   scripts/rebuild.sh v2.0.1
#
# 1. Moves the duckdb submodule to the given tag (kept shallow, with the tag fetched so that the build is labelled
#    with that version).
# 2. Builds the release shell, the test runner and the loadable extension for x86-64-v3, with every vcpkg dependency
#    built for the same baseline (vcpkg_triplets/x64-linux-v3.cmake).
# 3. Checks that the extension reports the requested version, and runs the test suite unless NO_TESTS=1.
#
# Environment:
#   VCPKG_TOOLCHAIN_PATH  vcpkg toolchain file (default: ~/vcpkg/scripts/buildsystems/vcpkg.cmake)
#   MARCH                 -march baseline (default: x86-64-v3)
#   NO_TESTS              skip the test suite when set to 1
#   MERGE_UPSTREAM        a ref to merge first, e.g. upstream/v2.0-cyanoptera (default: none)
set -euo pipefail

if [ $# -ne 1 ] || [[ ! "$1" =~ ^v[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
	echo "usage: $0 vX.Y.Z" >&2
	exit 2
fi
TAG="$1"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

export VCPKG_TOOLCHAIN_PATH="${VCPKG_TOOLCHAIN_PATH:-$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake}"
MARCH="${MARCH:-x86-64-v3}"

if [ -n "$(git status --porcelain --untracked-files=no)" ]; then
	echo "the working tree has uncommitted changes" >&2
	exit 1
fi

if [ -n "${MERGE_UPSTREAM:-}" ]; then
	git fetch upstream
	git merge --no-edit "$MERGE_UPSTREAM"
fi

git submodule update --init --depth 1 extension-ci-tools
git submodule update --init --depth 1 duckdb
(
	cd duckdb
	git fetch --depth 1 origin tag "$TAG"
	git checkout -q "$TAG"
	[ "$(git describe --tags)" = "$TAG" ]
)

if [ "$MARCH" = "x86-64-v3" ]; then
	export VCPKG_TARGET_TRIPLET=x64-linux-v3
	export VCPKG_HOST_TRIPLET=x64-linux-v3
	VCPKG_FLAGS="-DVCPKG_OVERLAY_TRIPLETS=$ROOT/vcpkg_triplets"
else
	VCPKG_FLAGS=""
fi

export EXT_FLAGS="${EXT_FLAGS:-} $VCPKG_FLAGS -DCMAKE_C_FLAGS=-march=$MARCH -DCMAKE_CXX_FLAGS=-march=$MARCH"
GEN=ninja make release

VERSION="$(./build/release/duckdb -noheader -list -c "SELECT extension_version FROM duckdb_extensions() WHERE extension_name = 'spatial'")"
LIBRARY="$(./build/release/duckdb -noheader -list -c "SELECT library_version FROM pragma_version()")"
if [ "$LIBRARY" != "$TAG" ]; then
	echo "built DuckDB reports $LIBRARY, expected $TAG" >&2
	exit 1
fi
echo "DuckDB $LIBRARY, spatial $VERSION, -march=$MARCH"

if [ "${NO_TESTS:-0}" != "1" ]; then
	./build/release/test/unittest "test/*"
fi

echo "extension: $ROOT/build/release/extension/spatial/spatial.duckdb_extension"
