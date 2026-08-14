# =============================================================================
# // BOMBERMAN // Game rendered using RayLib
# Detects if RayLib is found via pkg-config
# if not, downloaded and built locally into /deps folder
# Automatically discovers and builds all shared libraries and any binary found under src/
#
# Expected Layout:
#	assets/		game assets like art or sound
#	deps/		downlaoded third-party dependencies, such as raylib
#	build/		compiled outputs (auto-created)
#		/bin/	compiled binaries
#		/lib/	static libraries
#		/obj/	intermediate .o files
#	include		headers
#		/lib/	library-specific headers
#	src			.c source files
#		/lib/	library source files
#	tests		unit tests
#
# Usage:
#   make              runs 'all' by default, builds everything (downloads raylib if needed)
#   make deps         explicitly fetch and build missing dependencies only
#   make clean        wipe build/ and test bins (deps/ is kept)
#   make distclean    clean + wipe deps/ (forces re-download on next build)
#   make test			runs all tests
#	make unit			runs unit tests
#	make integration	runs integration tests
# =============================================================================

# Include corestack variables
include ../config.mk

# Bomb emoji
INIT_MSG     := \U0001F4A3
PROJECT_NAME := bomberman
DEPS_DIR := ./deps

# ----------------------------------------------------------------------
# Launcher
# Named binary in project root for easy access
# This is a symlink into build/bin so it always reflects latest build
# ----------------------------------------------------------------------
TARGET       := launcher
LAUNCHER_BIN := bombapp

# ----------------------------------------------------------------------
# RayLib Config
# 1) Use pkg-config to detect system-wide raylib install (via apt or brew)
# 2) If not found, download the source tarball and built it locally using raylib's own Makefile
# Result is a static library at deps/raylib-<ver>/src/librarylib.a
# ----------------------------------------------------------------------
RAYLIB_VERSION   := 6.0
RAYLIB_URL       := https://github.com/raysan5/raylib/archive/refs/tags/$(RAYLIB_VERSION).tar.gz
RAYLIB_LOCAL     := $(DEPS_DIR)/raylib-$(RAYLIB_VERSION)
RAYLIB_ARTIFACT  := $(RAYLIB_LOCAL)/src/libraylib.a
RAYLIB_BUILD_CMD := $(MAKE) -C $(RAYLIB_LOCAL)/src PLATFORM=PLATFORM_DESKTOP

# Platform-specific system libraries raylib depends on when linking statically.
# (pkg-config --libs includes these automatically for system installs.)
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin) # Apple
    RAYLIB_PLATFORM_LIBS := -framework OpenGL -framework Cocoa \
                            -framework IOKit -framework CoreAudio -framework CoreVideo
else # Linux
    RAYLIB_PLATFORM_LIBS := -lGL -ldl -lrt -lX11
endif

# Detect system raylib via pkg-config.
# $(shell ...) is evaluated at parse time; the result is 'yes' or empty.
RAYLIB_FOUND := $(shell pkg-config --exists raylib 2>/dev/null && echo yes)

# RAYLIB_DEP is either empty (system install, nothing to build) or points to
# the local static library (triggers the download+build rule)
ifeq ($(RAYLIB_FOUND),yes)
    # System raylib: let pkg-config provide the right flags automatically.
    RAYLIB_CFLAGS  := $(shell pkg-config --cflags raylib 2>/dev/null)
    RAYLIB_LDFLAGS := $(shell pkg-config --libs   raylib 2>/dev/null)
    RAYLIB_DEP     :=
    $(info raylib: Found via pkg-config! Using system install.)
else
    # No system raylib: build locally. RAYLIB_DEP triggers the build rule.
    RAYLIB_CFLAGS  := -I$(RAYLIB_LOCAL)/src
    RAYLIB_LDFLAGS := -L$(RAYLIB_LOCAL)/src -lraylib $(RAYLIB_PLATFORM_LIBS)
    RAYLIB_DEP     := $(RAYLIB_ARTIFACT)
    $(info raylib: Not found! Will build locally in $(RAYLIB_LOCAL))
endif

# The name of the binaries (matching a src/ directory or flat file) that
# links raylib. Only this target receives the extra RAYLIB_* flags
# Space-separated list! (eg: EXTRA_DEP_BINS := bombapp bombui)
# bombd needs it too: libbombbrain's collision/movement math (Vector2Add etc)
# are real raylib symbols in this raylib build, not header-only inlines
EXTRA_DEP_BINS    := bombapp bombd
EXTRA_DEP_CFLAGS  := $(RAYLIB_CFLAGS)
EXTRA_DEP_LDFLAGS := $(RAYLIB_LDFLAGS)
EXTRA_DEP_DEPS    := $(RAYLIB_DEP)

include ../common.mk
include ../common-test.mk
include ../common-deps.mk

# The name of the binaries (matching a src/ directory or flat file) that
# links raylib. Only this target receives the extra RAYLIB_* flags
# Space-separated list! (eg: RAYLIB_BINS := bombapp bombui)
RAYLIB_BINS := bombapp bombd

# Generate template for fetching and building raylib
# NOT cleaned by make clean so we don't have to re-download each time
$(eval $(call FETCH_TARBALL_template,RAYLIB))

$(TARGET): $(BIN_OUT)/$(LAUNCHER_BIN)
	ln -sf $(BIN_OUT)/$(LAUNCHER_BIN) $(TARGET)

.PHONY: run deps distclean kill-server

run: $(TARGET)
	./$(TARGET)

# Fetch and build dependencies without building the project
deps: $(RAYLIB_DEP)
	@echo "==> All dependencies ready."

# ----------------------------------------------------------------------
# Cleaning
# ----------------------------------------------------------------------
# Keeps /deps intact while cleaning build artifacts
clean:
	rm -rf ./build $(UNIT_BIN)
	rm -f $(TARGET)

# Remove /deps and build artifacts
distclean: 
	clean
	rm -rf $(DEPS_DIR)

SERVER_BIN := bombd
CLIENT_BIN := bombu
SERVER_PORT := 6700

kill-server:
	@pkill -9 -x "${SERVER_BIN}" 2>/dev/null; \
	fuser -k ${SERVER_PORT}/tcp 2>/dev/null; \
	echo "Killed any ${SERVER_BIN} process and freed port ${SERVER_PORT}."