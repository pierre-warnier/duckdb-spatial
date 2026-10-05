PROJ_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

# Configuration of extension
EXT_NAME=spatial
EXT_CONFIG=${PROJ_DIR}extension_config.cmake

TEST_FLAGS:=--batch-size 1 --batch-timeout 300

# Stabilize all tests in CI
ifdef CI
TEST_FLAGS+= --stabilize-tests
endif

T ?= $(TEST_FLAGS) "test/*"

# Include the Makefile from extension-ci-tools
include extension-ci-tools/makefiles/duckdb_extension.Makefile

unittest_relassert:
	build/relassert/test/run $(T)


#### Override the included format target because we have different source tree layout
format:
	find src/spatial -iname *.hpp -o -iname *.cpp | xargs clang-format --sort-includes=0 -style=file -i
	cmake-format -i CMakeLists.txt

#### Install the locally built (unsigned) extension into an extension directory of its own, never the shared default one
LOCAL_EXTENSION_DIRECTORY ?= $(HOME)/.duckdb/extensions-spatial-fork
install-local:
	./build/release/duckdb -unsigned -c "SET extension_directory = '$(LOCAL_EXTENSION_DIRECTORY)'; FORCE INSTALL spatial FROM '$(PROJ_DIR)build/release/repository';"
	@echo "Installed into $(LOCAL_EXTENSION_DIRECTORY): open connections with extension_directory set to it and unsigned extensions allowed"
