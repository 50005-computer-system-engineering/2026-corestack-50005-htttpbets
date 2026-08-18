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
#   make              	runs 'all' by default, builds everything
#   make run/run-server/run-client	launch server/client in a new terminal
#   make test			runs all tests
#	make unit			runs unit tests
#	make integration	runs integration tests
# 	make clean 			wipe build/ folder
#	make lint			automatically format code to style guide
# =============================================================================
# Include corestack variables
include ../config.mk

# Block Emoji
INIT_MSG     := \U0001F9F1 
PROJECT_NAME := tetris
POST_INIT    := mkdir -p var/log var/run

SERVER_BIN := tetrisd
CLIENT_BIN := tetrisu

# Include basic compiling logic
include ../common.mk
# Include testing logic
include ../common-test.mk
# ----------------------------------------------------------------------
# Launcher
# ----------------------------------------------------------------------
# Dynamically find available terminal emulator
TERM_CMD := $(shell which kitty || which gnome-terminal || which xfce4-terminal || which xterm)

.PHONY: run run-server run-client

run-server:
	${TERM_CMD} -- bash -c "${BIN_OUT}/${SERVER_BIN}; exec bash" &

run-client:
	${TERM_CMD} -- bash -c "${BIN_OUT}/${CLIENT_BIN}; exec bash" &

run:
	${TERM_CMD} -- bash -c "${BIN_OUT}/${SERVER_BIN}; exec bash" &
	sleep 1;
	${TERM_CMD} -- bash -c "${BIN_OUT}/${CLIENT_BIN}; exec bash" &