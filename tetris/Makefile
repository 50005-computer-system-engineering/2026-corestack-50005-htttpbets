# =============================================================================
# // TETRIS // Game rendered using custom graphics library
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
#   make              	runs 'all' by default, builds everything (downloads stb_image if needed)
#   make deps			explicitly fetch missing dependencies only
#   make run/run-server/run-client	launch server/client in a new terminal
#   make test			runs all tests
#	make unit			runs unit tests
#	make integration	runs integration tests
# 	make clean 			wipe build/ folder
#	make distclean		clean + wipe deps/ (forces re-download on next build)
#	make lint			automatically format code to style guide
# =============================================================================
# Include corestack variables
include ../config.mk

# Block Emoji
INIT_MSG     := \U0001F9F1
PROJECT_NAME := tetris
POST_INIT    := mkdir -p var/log var/run
DEPS_DIR     := ./deps

# stb_image (single-header image loader)
# Used by libgraphics/viuer.c to decode PNGs/JPEGs/BMPs/TGAs
# Ref: https://github.com/nothings/stb
STB_IMAGE_URL      := https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
STB_IMAGE_ARTIFACT := $(DEPS_DIR)/stb_image/stb_image.h

COMMON_CFLAGS += -I$(dir $(STB_IMAGE_ARTIFACT))

# Include basic compiling logic
include ../common.mk
# Include testing logic
include ../common-test.mk
# Include dependency downloading logic
include ../common-deps.mk

# Generate template for fetching stb_image.h
# NOT cleaned by make clean so we don't have to re-download each time
$(eval $(call FETCH_FILE_template,STB_IMAGE))

# libgraphics's viuer.c #includes stb_image.h, so it must be fetched first
$(OBJ_DIR)/libgraphics/viuer.o: $(STB_IMAGE_ARTIFACT)

.PHONY: deps distclean

# Fetch dependencies without building the project
deps: $(STB_IMAGE_ARTIFACT)
	@echo "==> All dependencies ready."

# Remove /deps and build artifacts
distclean: clean
	rm -rf $(DEPS_DIR)

# ----------------------------------------------------------------------
# Launcher
# ----------------------------------------------------------------------
# Dynamically find available terminal emulator
TERM_CMD := $(shell which kitty || which gnome-terminal || which xfce4-terminal || which xterm)

# Binaries
SERVER_BIN := tetrisd
CLIENT_BIN := tetrisu
SERVER_PORT := 6700

.PHONY: run run-server run-client kill-server

run-server:
	${TERM_CMD} -- bash -c "${BIN_OUT}/${SERVER_BIN}; exec bash" &

run-client:
	${TERM_CMD} -- bash -c "${BIN_OUT}/${CLIENT_BIN}; exec bash" &

run:
	${TERM_CMD} -- bash -c "${BIN_OUT}/${SERVER_BIN}; exec bash" &
	sleep 1;
	${TERM_CMD} -- bash -c "${BIN_OUT}/${CLIENT_BIN}; exec bash" &

kill-server:
	@pkill -9 -x "${SERVER_BIN}" 2>/dev/null; \
	fuser -k ${SERVER_PORT}/tcp 2>/dev/null; \
	echo "Killed any ${SERVER_BIN} process and freed port ${SERVER_PORT}."