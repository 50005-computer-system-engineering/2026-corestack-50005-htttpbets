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
SERVER_BIN := bombd
CLIENT_BIN := bombu

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
# Space-separated list!
EXTRA_DEP_BINS    := bombapp bombd
EXTRA_DEP_CFLAGS  := $(RAYLIB_CFLAGS)
EXTRA_DEP_LDFLAGS := $(RAYLIB_LDFLAGS)
EXTRA_DEP_DEPS    := $(RAYLIB_DEP)

include ../common.mk
include ../common-test.mk
include ../common-deps.mk

# Generate template for fetching and building raylib
# NOT cleaned by make clean so we don't have to re-download each time
$(eval $(call FETCH_TARBALL_template,RAYLIB))

$(TARGET): $(BIN_OUT)/$(LAUNCHER_BIN)
	ln -sf $(BIN_OUT)/$(LAUNCHER_BIN) $(TARGET)

.PHONY: run-server run-client deps distclean

# Dynamically find available terminal emulator
TERM_CMD := $(shell which kitty || which gnome-terminal || which xfce4-terminal || which xterm)

# Launch dir up as our certificates are read from CWD
run-server: $(BIN_OUT)/$(SERVER_BIN)
	${TERM_CMD} -- bash -c "cd $(CURDIR)/.. && ./bomberman/$(BIN_OUT)/$(SERVER_BIN); exec bash" &

run-client: $(BIN_OUT)/$(CLIENT_BIN)
	${TERM_CMD} -- bash -c "$(CURDIR)/$(BIN_OUT)/$(CLIENT_BIN); exec bash" &

# Fetch and build dependencies without building the project
deps: $(RAYLIB_DEP)
	@echo "==> All dependencies ready."

# ----------------------------------------------------------------------
# Game-logic fuzzer (libbombbrain) & Valgrind
# unit/integration/test targets come from common-test.mk
# ----------------------------------------------------------------------
FUZZ_DIR := $(TESTS_DIR)/fuzz
FUZZ_BIN := $(FUZZ_DIR)/bin

$(FUZZ_BIN)/fuzz_harness: $(FUZZ_DIR)/fuzz_harness.c | $(ALL_LIB_ARCHIVES)
	@mkdir -p $(FUZZ_BIN)
	$(CC) $(TEST_CFLAGS) $< -o $@ $(TEST_LDFLAGS) $(TEST_LDLIBS) $(RAYLIB_LDFLAGS)

fuzz: $(FUZZ_BIN)/fuzz_harness
	@echo "==> Executing State Consistency Fuzzer"
	./$(FUZZ_BIN)/fuzz_harness
	@echo "==> Executing Valgrind Memory Safety Profiler"
	valgrind --leak-check=full --track-origins=yes ./$(FUZZ_BIN)/fuzz_harness

# --- Master Test Target ---
test: unit integration fuzz

# --- bombfuzzer: custom adversarial-input fuzzer for corestack (docs/THREAT_MODEL.md 2.4) ---
# Targets the shared libraries bombd will be built on (libbattleroyale/libhypertext/libhtttp) -
# not tetrisd or any other bomberman-external binary. Kept out of `unit integration` -> `test`
# since its Valgrind passes are slow - run explicitly with `make bombfuzzer`.
BOMBFUZZER_DIR    := $(TESTS_DIR)/bombfuzzer
BOMBFUZZER_BIN    := $(BOMBFUZZER_DIR)/bin
BOMBFUZZER_COMMON := $(BOMBFUZZER_DIR)/bombfuzzer_common.c

# message.h/common.h (libbattleroyale) are private headers, not under CS_INC
BOMBFUZZER_CFLAGS := $(TEST_CFLAGS) -I$(CORESTACK_DIR)/src/lib/libbattleroyale
BOMBFUZZER_SRCS   := bombfuzzer_bytes bombfuzzer_message bombfuzzer_hypertext bombfuzzer_htttp bombfuzzer_stress
# only needs corestack, not $(LDLIBS)'s default -lbombbrain (bomberman's own game logic)
BOMBFUZZER_LDLIBS := $(CS_LDLIBS) $(SYS_LDLIBS)

$(CS_LIB)/libbattleroyale.a:
	$(MAKE) -C $(CORESTACK_DIR)

$(BOMBFUZZER_BIN)/%: $(BOMBFUZZER_DIR)/%.c $(BOMBFUZZER_COMMON) | $(CS_LIB)/libbattleroyale.a
	@mkdir -p $(BOMBFUZZER_BIN)
	$(CC) $(BOMBFUZZER_CFLAGS) $< $(BOMBFUZZER_COMMON) -o $@ $(TEST_LDFLAGS) $(BOMBFUZZER_LDLIBS)

# `|| echo ...` keeps one crashing layer from hiding whether the others are clean
VALGRIND := valgrind --leak-check=full --track-origins=yes

bombfuzzer: $(addprefix $(BOMBFUZZER_BIN)/,$(BOMBFUZZER_SRCS))
	@mkdir -p $(BOMBFUZZER_DIR)/findings
	@echo "==> bombfuzzer: Adversarial Input Handling (THREAT_MODEL.md 2.4)"
	./$(BOMBFUZZER_BIN)/bombfuzzer_bytes || echo "==> WARNING: bombfuzzer_bytes crashed"
	./$(BOMBFUZZER_BIN)/bombfuzzer_message || echo "==> WARNING: bombfuzzer_message crashed"
	./$(BOMBFUZZER_BIN)/bombfuzzer_hypertext || echo "==> WARNING: bombfuzzer_hypertext crashed"
	./$(BOMBFUZZER_BIN)/bombfuzzer_htttp || echo "==> WARNING: bombfuzzer_htttp crashed"
	@echo "==> bombfuzzer: concurrency/volume stress (local precursor - no live server to load-test yet)"
	./$(BOMBFUZZER_BIN)/bombfuzzer_stress || echo "==> WARNING: bombfuzzer_stress crashed"
	@echo "==> bombfuzzer: Memory Safety under Valgrind (THREAT_MODEL.md 2.2)"
	$(VALGRIND) ./$(BOMBFUZZER_BIN)/bombfuzzer_bytes 2>&1 | tee $(BOMBFUZZER_DIR)/findings/valgrind_bytes.log \
		|| echo "==> WARNING: bombfuzzer_bytes crashed under Valgrind - see findings/valgrind_bytes.log"
	$(VALGRIND) ./$(BOMBFUZZER_BIN)/bombfuzzer_message 2>&1 | tee $(BOMBFUZZER_DIR)/findings/valgrind_message.log \
		|| echo "==> WARNING: bombfuzzer_message crashed under Valgrind - see findings/valgrind_message.log"
	$(VALGRIND) ./$(BOMBFUZZER_BIN)/bombfuzzer_hypertext 2>&1 | tee $(BOMBFUZZER_DIR)/findings/valgrind_hypertext.log \
		|| echo "==> WARNING: bombfuzzer_hypertext crashed under Valgrind - see findings/valgrind_hypertext.log"
	$(VALGRIND) ./$(BOMBFUZZER_BIN)/bombfuzzer_htttp 2>&1 | tee $(BOMBFUZZER_DIR)/findings/valgrind_htttp.log \
		|| echo "==> WARNING: bombfuzzer_htttp crashed under Valgrind - see findings/valgrind_htttp.log"
	@# scaled down from the plain run's defaults - Valgrind measured ~22ms/op here, vs <1ms without it
	BOMBFUZZER_STRESS_THREADS=8 BOMBFUZZER_STRESS_ITERS=50 $(VALGRIND) ./$(BOMBFUZZER_BIN)/bombfuzzer_stress 2>&1 \
		| tee $(BOMBFUZZER_DIR)/findings/valgrind_stress.log \
		|| echo "==> WARNING: bombfuzzer_stress crashed under Valgrind - see findings/valgrind_stress.log"
	@echo "==> bombfuzzer: findings (if any) logged under $(BOMBFUZZER_DIR)/findings/"

# --- bombfuzzer coverage report (node/edge coverage via gcov) ---
# Rebuilds message.c and libhypertext.c with --coverage, linked against their harness. Each pair
# builds into its own subdirectory so their .gcno files can't collide with each other.
COVERAGE_DIR          := $(BOMBFUZZER_DIR)/coverage
COVERAGE_LIBBR         := $(CORESTACK_DIR)/src/lib/libbattleroyale
COVERAGE_LIBHYPERTEXT := $(CORESTACK_DIR)/src/lib/libhypertext.c

define coverage_pair
@mkdir -p $(COVERAGE_DIR)/$(1)
$(CC) $(BOMBFUZZER_CFLAGS) --coverage -O0 -c $(BOMBFUZZER_DIR)/bombfuzzer_$(1).c -o $(COVERAGE_DIR)/$(1)/bombfuzzer_$(1).o
$(CC) $(BOMBFUZZER_CFLAGS) --coverage -O0 -c $(BOMBFUZZER_COMMON) -o $(COVERAGE_DIR)/$(1)/bombfuzzer_common.o
$(CC) $(BOMBFUZZER_CFLAGS) --coverage -O0 -c $(2) -o $(COVERAGE_DIR)/$(1)/$(3).o
$(CC) --coverage $(COVERAGE_DIR)/$(1)/bombfuzzer_$(1).o $(COVERAGE_DIR)/$(1)/bombfuzzer_common.o $(COVERAGE_DIR)/$(1)/$(3).o \
	-o $(COVERAGE_DIR)/$(1)/coverage_run -lpthread
cd $(COVERAGE_DIR)/$(1) && ./coverage_run
@echo "==> bombfuzzer-coverage: node/edge coverage of $(3).c reached by bombfuzzer_$(1)"
cd $(COVERAGE_DIR)/$(1) && gcov $(3).o
endef

bombfuzzer-coverage:
	@echo "==> bombfuzzer-coverage: building message.c + libhypertext.c with --coverage"
	$(call coverage_pair,message,$(COVERAGE_LIBBR)/message.c,message)
	$(call coverage_pair,hypertext,$(COVERAGE_LIBHYPERTEXT),libhypertext)

# ----------------------------------------------------------------------
# Cleaning
# ----------------------------------------------------------------------
# Keeps /deps intact while cleaning build artifacts
clean:
	rm -rf ./build $(UNIT_BIN)
	rm -f $(TARGET)

# Remove /deps and build artifacts
distclean: clean
	rm -rf $(DEPS_DIR)

SERVER_BIN := bombd
CLIENT_BIN := bombu
SERVER_PORT := 6700

kill-server:
	@pkill -9 -x "${SERVER_BIN}" 2>/dev/null; \
	fuser -k ${SERVER_PORT}/tcp 2>/dev/null; \
	echo "Killed any ${SERVER_BIN} process and freed port ${SERVER_PORT}."