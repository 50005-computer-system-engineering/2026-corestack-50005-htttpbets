# =============================================================================
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

# ----------------------------------------------------------------------
# Toolchain
# ----------------------------------------------------------------------
# GCC Compiler
CC := gcc

# Archiver that builds .o files into static library
# r = insert/replace, c=create if missing, s=add symbol index
AR := ar rcs

# ----------------------------------------------------------------------
# Directories
# ----------------------------------------------------------------------
SRC_DIR    := ./src
LIB_SUBDIR := lib
INC_DIR    := include
OBJ_DIR    := ./build/obj
LIB_OUT    := ./build/lib
BIN_OUT    := ./build/bin
DEPS_DIR   := ./deps

# ----------------------------------------------------------------------
# Launcher
# Named binary in project root for easy access
# This is a symlink into build/bin so it always reflects latest build
# ----------------------------------------------------------------------
TARGET       := launcher
# Main entry point
LAUNCHER_BIN := bombapp

$(TARGET): $(BIN_OUT)/$(LAUNCHER_BIN)
	ln -sf $(BIN_OUT)/$(LAUNCHER_BIN) $(TARGET)

run: $(TARGET)
	./$(TARGET)

# ----------------------------------------------------------------------
# Compiler Flags
# ----------------------------------------------------------------------
# -I$(INC_DIR)                find headers in include/
# -I$(INC_DIR)/$(LIB_SUBDIR)  find headers in include/lib/ (library-specific headers)
# -I$(CS_INC)                 also search corestack/include/ for shared library headers
# $(COMMON_CFLAGS)            expands to: -Wall -Wextra -g -pthread (defined in config file)
CFLAGS  := -I$(INC_DIR) -I$(INC_DIR)/$(LIB_SUBDIR) -I$(CS_INC) $(COMMON_CFLAGS)

# LDFLAGS: flags for the linker (search paths and system libs)
# CSLIB comes from included config file
# LDLIBS is filled in after library auto-discovery below!
LDFLAGS := -L$(LIB_OUT) -L$(CS_LIB) -lpthread

# ----------------------------------------------------------------------
# RayLib Config
# 1) Use pkg-config to detect system-wide raylib install (via apt or brew)
# 2) If not found, download the source tarball and built it locally using raylib's own Makefile
# Result is a static library at deps/raylib-<ver>/src/librarylib.a
# ----------------------------------------------------------------------
RAYLIB_VERSION := 6.0
RAYLIB_URL     := https://github.com/raysan5/raylib/archive/refs/tags/$(RAYLIB_VERSION).tar.gz
RAYLIB_LOCAL   := $(DEPS_DIR)/raylib-$(RAYLIB_VERSION)
RAYLIB_STATIC  := $(RAYLIB_LOCAL)/src/libraylib.a

# Platform-specific system libraries raylib depends on when linking statically.
# (pkg-config --libs includes these automatically for system installs.)
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
    RAYLIB_PLATFORM_LIBS := -framework OpenGL -framework Cocoa \
                            -framework IOKit -framework CoreAudio -framework CoreVideo
else
    RAYLIB_PLATFORM_LIBS := -lGL -lm -ldl -lrt -lX11
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
    RAYLIB_DEP     := $(RAYLIB_STATIC)
    $(info raylib: Not found! Will build locally in $(RAYLIB_LOCAL))
endif

# The name of the binaries (matching a src/ directory or flat file) that
# links raylib. Only this target receives the extra RAYLIB_* flags
# Space-separated list! (eg: RAYLIB_BINS := bombapp bombui)
RAYLIB_BINS := bombapp

# ----------------------------------------------------------------------
# RayLib Download & Build Rule
# Only runs when RAYLIB_DEP is non-empty (ie: no system install)
# 1) Download source tarball using curl/wget
# 2) Build static library using raylib's own Makefile
# 3) Store result (libraylib.a) in deps/
# This is NOT cleaned by 'make clean' so we don't have to re-download every clean
# Use make distclean to clean everything
# ----------------------------------------------------------------------
$(RAYLIB_STATIC):
	@echo "==> Downloading raylib $(RAYLIB_VERSION)..."
	@mkdir -p $(DEPS_DIR)
	@if command -v curl > /dev/null 2>&1; then \
	    curl -fsSL $(RAYLIB_URL) | tar -xz -C $(DEPS_DIR); \
	elif command -v wget > /dev/null 2>&1; then \
	    wget -qO- $(RAYLIB_URL) | tar -xz -C $(DEPS_DIR); \
	else \
	    echo "Error: neither curl nor wget found."; \
	    echo "Install one, or install raylib manually and re-run make."; \
	    exit 1; \
	fi
	@echo "==> Building raylib $(RAYLIB_VERSION) (this may take a moment)..."
	$(MAKE) -C $(RAYLIB_LOCAL)/src PLATFORM=PLATFORM_DESKTOP
	@echo "==> raylib ready at $(RAYLIB_STATIC)"

# Convenience target: fetch and build dependencies without building the project.
deps: $(RAYLIB_DEP)
	@echo "==> All dependencies ready."

# ----------------------------------------------------------------------
# Library Auto-Discovery (src/lib/<libname>)
# Scans src/lib for subdirectories/files, and builds them as static libraries
# ----------------------------------------------------------------------
# ** Directory-based libs (eg: src/lib/libtest/ -> build/lib/libtest.a) **
# Lists every subdirectory and strips trailing slash off each result
_LIB_SRCDIRS_ALL := $(patsubst %/,%,$(wildcard $(SRC_DIR)/$(LIB_SUBDIR)/*/))
# Loops over every directory found and drop those without .c files
_LIB_SRCDIRS     := $(foreach d,$(_LIB_SRCDIRS_ALL),$(if $(wildcard $(d)/*.c),$(d)))
# Get folder names by stripping leading path to each directory
LIB_NAMES        := $(notdir $(_LIB_SRCDIRS))

# ** Flat File libs (eg: src/lib/libtest.c -> build/lib/libtest.a) **
# Lists every .c file sitting directly in /src/lib/
FLAT_LIB_SRCS  := $(wildcard $(SRC_DIR)/$(LIB_SUBDIR)/*.c)
# Get file names by stripping leading path and .c extension from each file
FLAT_LIB_NAMES := $(patsubst $(SRC_DIR)/$(LIB_SUBDIR)/%.c,%,$(FLAT_LIB_SRCS))

# ** Linker Flags **
# Converts every library name into a -l flag, stripping 'lib' prefix (eg: libtest -> -ltest)
LOCAL_LDLIBS := $(patsubst lib%,-l%,$(LIB_NAMES))
# Assigns final set of -l flags to LDLIBS, including corestack libraries
LDLIBS       := $(LOCAL_LDLIBS) $(CS_LDLIBS) -lm -lcmocka

# Final container of all libraries, empty for now
ALL_LIB_ARCHIVES :=

# Directory Library Template: Subdirectory -> Archive
# $(1) is the argument supplied to this function, the library name (eg: libtest)
# Note that we include each directory's own path to the CFLAGS to
# (eg: Allow src/lib/libhtttp/client.c to #include "client.h" instead of "libhtttp/client.h")
# 1) Convert every .c file found in directory to an object file (eg: src/lib/libtest/test.c -> build/obj/libtest/test.o)
# 2) Compile corresponding .c file whenever a .o is needed, creating the directory if hasn't existed yet
# 3) Bundle all .o files into the static archive (eg: build/lib/libtest.a), where $$@ is the archive, $$^ is all the .o files
# 4) Append built archive to running list of libraries to build at top-level
# See: https://devdocs.io/gnu_make/automatic-variables (we write $$ to escape inside a define block)
define LIB_template =
$(1)_SRC := $$(wildcard $(SRC_DIR)/$(LIB_SUBDIR)/$(1)/*.c)
$(1)_OBJ := $$(patsubst $(SRC_DIR)/$(LIB_SUBDIR)/$(1)/%.c, \
             $(OBJ_DIR)/$(1)/%.o,$$($(1)_SRC))
$(1)_CFLAGS := $(CFLAGS) -I$(INC_DIR)/$(LIB_SUBDIR)/$(1)

$(OBJ_DIR)/$(1)/%.o: $(SRC_DIR)/$(LIB_SUBDIR)/$(1)/%.c
	@mkdir -p $$(dir $$@)
	$(CC) $$($(1)_CFLAGS)  -c $$< -o $$@

$(LIB_OUT)/$(1).a: $$($(1)_OBJ)
	@mkdir -p $(LIB_OUT)
	$(AR) $$@ $$^

ALL_LIB_ARCHIVES += $(LIB_OUT)/$(1).a
endef

# Runs template once per directory library name
$(foreach lib,$(LIB_NAMES),$(eval $(call LIB_template,$(lib))))

# Flat Library Template: Single .c file -> Archive
# 1) Compile single flat source file directory into a .o
# 2) Archive the single .o into .a
# 3) Append built archive to running list of libraries to build
define FLAT_LIB_template =
$(OBJ_DIR)/$(1).o: $(SRC_DIR)/$(LIB_SUBDIR)/$(1).c
	@mkdir -p $$(dir $$@)
	$(CC) $(CFLAGS) -c $$< -o $$@

$(LIB_OUT)/$(1).a: $(OBJ_DIR)/$(1).o
	@mkdir -p $(LIB_OUT)
	$(AR) $$@ $$^

ALL_LIB_ARCHIVES += $(LIB_OUT)/$(1).a
endef

# Runs template once per flat library
$(foreach lib,$(FLAT_LIB_NAMES),$(eval $(call FLAT_LIB_template,$(lib))))

# ----------------------------------------------------------------------
# Binary Auto-Discovery (src/<name>)
# Builds everything under src/ that is NOT inside src/lib as binaries
# ----------------------------------------------------------------------
# ** Directory-based Binaries (eg: src/test/ -> build/bin/test) **
# Lists every subdirectory inside src/ and strips trailing slash off each result
_ALL_SRCDIRS_ALL := $(patsubst %/,%,$(wildcard $(SRC_DIR)/*/))
# Drop any directories inside /lib folder
_BIN_SRCDIRS_ALL := $(filter-out $(SRC_DIR)/$(LIB_SUBDIR),$(_ALL_SRCDIRS_ALL))
# Drop any directories without .c files
_BIN_SRCDIRS     := $(foreach d,$(_BIN_SRCDIRS_ALL),$(if $(wildcard $(d)/*.c),$(d)))
# Get file names by stripping leading paths
BIN_NAMES        := $(notdir $(_BIN_SRCDIRS))

# ** Flat File Binaries (eg: src/test.c -> build/bin/test) **
# Every .c file sitting directly in /src
FLAT_BIN_SRCS  := $(wildcard $(SRC_DIR)/*.c)
# Get file names by stripping leading path and .c extension from each file
FLAT_BIN_NAMES := $(patsubst $(SRC_DIR)/%.c,%,$(FLAT_BIN_SRCS))

ALL_BINS :=

# Directory Binary Template: Subdirectory -> Binary
# Extra Variables set for binaries that require raylib, empty strings for others
# $(1)_EXTRA_CFLAGS		adds raylib's include path (-I) only for that binary.
# $(1)_EXTRA_LDFLAGS 	adds -lraylib and platform libs only for that binary.
# $(1)_EXTRA_DEPS		if raylib needs to be built locally, makes the binary
#                      	depend on the local libraylib.a (order-only).
#                      	Empty for system installs. Nothing extra to build.
# 
# Similar to library templates.
# 1) Create list of every .c file found in directory
# 2) Declare binary as target with prerequisites src (normal), libraries & extra deps (order-only)
# ie: src is rebuilt whenever it's modified, libraries/deps are rebuilt only if its existence changes
# See: https://devdocs.io/gnu_make/prerequisite-types
# 3) Also include each directory's own path to the search path 
# (eg: Allow src/tetrisu/test.c to #include "test.h" instead of "tetrisu/test.h")
# 4) Compile every file to binary, including linker paths
# 5) Append compiled binary to running list of binaries to build at top-level
define BIN_template =
$(1)_SRC          := $$(wildcard $(SRC_DIR)/$(1)/*.c)
$(1)_CFLAGS := $(CFLAGS) -I$(INC_DIR)/$(1)
$(1)_EXTRA_CFLAGS  := $$(if $$(filter $(RAYLIB_BINS),$(1)),$(RAYLIB_CFLAGS))
$(1)_EXTRA_LDFLAGS := $$(if $$(filter $(RAYLIB_BINS),$(1)),$(RAYLIB_LDFLAGS))
$(1)_EXTRA_DEPS    := $$(if $$(filter $(RAYLIB_BINS),$(1)),$(RAYLIB_DEP))

$(BIN_OUT)/$(1): $$($(1)_SRC) | $(ALL_LIB_ARCHIVES) $$($(1)_EXTRA_DEPS)
	@mkdir -p $$(dir $$@)
	$(CC) $$($(1)_CFLAGS) $$($(1)_EXTRA_CFLAGS) $$($(1)_SRC) -o $$@ \
		$(LDFLAGS) $(LDLIBS) $$($(1)_EXTRA_LDFLAGS)

ALL_BINS += $(BIN_OUT)/$(1)
endef

# Runs template once per directory binary name
$(foreach bin,$(BIN_NAMES),$(eval $(call BIN_template,$(bin))))

# Flat Binary Template: Single .c File -> Binary
# Identical to BIN_template except source is a single named file ($$<) rather than 
# a wildcard over a directory
define FLAT_BIN_template =
$(1)_EXTRA_CFLAGS  := $$(if $$(filter $(RAYLIB_BIN),$(1)),$(RAYLIB_CFLAGS))
$(1)_EXTRA_LDFLAGS := $$(if $$(filter $(RAYLIB_BIN),$(1)),$(RAYLIB_LDFLAGS))
$(1)_EXTRA_DEPS    := $$(if $$(filter $(RAYLIB_BIN),$(1)),$(RAYLIB_DEP))

$(BIN_OUT)/$(1): $(SRC_DIR)/$(1).c | $(ALL_LIB_ARCHIVES) $$($(1)_EXTRA_DEPS)
	@mkdir -p $$(dir $$@)
	$(CC) $(CFLAGS) $$($(1)_EXTRA_CFLAGS) $$< -o $$@ \
		$(LDFLAGS) $(LDLIBS) $$($(1)_EXTRA_LDFLAGS)

ALL_BINS += $(BIN_OUT)/$(1)
endef

# Runs template once per flat binary
$(foreach bin,$(FLAT_BIN_NAMES),$(eval $(call FLAT_BIN_template,$(bin))))

# ----------------------------------------------------------------------
# Top-Level Targets
# ----------------------------------------------------------------------
.PHONY: all init deps run unit integration test clean clean-deps distclean

# 'all' is ran by default when make is ran with no arguments
.DEFAULT_GOAL := all
all: $(ALL_LIB_ARCHIVES) $(ALL_BINS) $(TARGET)

# Print a Bomb emoji for Bomberman :D
# TODO: Implement pretty-print for other commands
init: 
	make clean
	bear -- make
	@printf "\U0001F4A3 Created compile_commands for Intellisense!\n"

# ----------------------------------------------------------------------
# Testing
# Placeholder for now, to change later!
# ----------------------------------------------------------------------
TESTS_DIR := ./tests
UNIT_DIR  := $(TESTS_DIR)/unit
INT_DIR   := $(TESTS_DIR)/integration
UNIT_BIN  := $(UNIT_DIR)/bin

UNIT_SRCS := $(wildcard $(UNIT_DIR)/test_*.c)
UNIT_BINS := $(UNIT_SRCS:$(UNIT_DIR)/%.c=$(UNIT_BIN)/%)

# Tests never link raylib — test game logic through libbombbrain directly.
TEST_CFLAGS  := $(CFLAGS) $(_EXTRA_CFLAGS)
TEST_LDFLAGS := -L$(LIB_OUT) -L$(CS_LIB) -lpthread
TEST_LDLIBS  := $(LDLIBS)

$(UNIT_BIN)/test_%: $(UNIT_DIR)/test_%.c | $(ALL_LIB_ARCHIVES)
	@mkdir -p $(UNIT_BIN)
	$(CC) $(TEST_CFLAGS) $< -o $@ $(TEST_LDFLAGS) $(TEST_LDLIBS)

unit: $(UNIT_BINS)
	@echo "==> Running bomberman unit tests"
	@pass=0; fail=0; \
	for t in $(UNIT_BINS); do \
	  echo "--- $$t ---"; \
	  if $$t; then pass=$$((pass+1)); else fail=$$((fail+1)); fi; \
	done; \
	echo ""; \
	echo "Unit tests: $$pass passed, $$fail failed"; \
	test $$fail -eq 0

integration: all
	@echo "==> Running bomberman integration tests"
	@pass=0; fail=0; \
	for s in $(INT_DIR)/*.sh; do \
	  [ -f "$$s" ] || continue; \
	  echo "--- $$s ---"; \
	  if bash $$s; then pass=$$((pass+1)); else fail=$$((fail+1)); fi; \
	done; \
	echo ""; \
	echo "Integration tests: $$pass passed, $$fail failed"; \
	test $$fail -eq 0

test: unit integration

# ----------------------------------------------------------------------
# Cleaning
# ----------------------------------------------------------------------
# Keeps /deps intact while cleaning build artifacts
clean:
	rm -rf ./build $(UNIT_BIN)
	rm -f $(TARGET)

# Remove /deps and build artifacts
distclean: 
	clean clean-deps
	rm -rf $(DEPS_DIR)
