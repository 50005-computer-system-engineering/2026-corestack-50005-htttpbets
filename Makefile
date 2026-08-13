# ======================================================================
# Root Makefile: Builds all three subprojects in the correct order.
# Usage:
#	make 			build everything
#	make init		runs bear -- make on every subfolder to prepare intellisense
#   make tetris 	build corestack + tetris only
#   make bomberman 	build corestack + bomberman only
#   make test 		run all tests across all projects
#   make clean 		wipe all build artifacts
#	make distclean 	wipe everything including dependencies downloaded
# ======================================================================

.PHONY: all corestack tetris bomberman test clean distclean lint

all: corestack tetris bomberman

init:
	$(MAKE) -C corestack init
	$(MAKE) -C tetris init
	$(MAKE) -C bomberman init

corestack: clean
	$(MAKE) -C corestack

tetris: corestack
	$(MAKE) -C tetris

bomberman: corestack
	$(MAKE) -C bomberman

# Runs all tests across all 3 projects
test: all
	$(MAKE) -C corestack test
	$(MAKE) -C tetris    test
	$(MAKE) -C bomberman test

# Remove all compiled output
clean:
	$(MAKE) -C corestack clean
	$(MAKE) -C tetris    clean
	$(MAKE) -C bomberman clean

# Also remove dependencies (eg: raylib build)
distclean: clean
	$(MAKE) -C bomberman distclean

# Run clang-format (spacing & brackets) on all source files
format:
	${MAKE} -C corestack format
	${MAKE} -C tetris    format
	${MAKE} -C bomberman format

# Fix variable naming via clang-tidy, 
# then running clang-format to fix layout (spacing & brackets)
# tidy > format as renames can change line lengths
lint:
	${MAKE} -C corestack lint
	${MAKE} -C tetris    lint
	${MAKE} -C bomberman lint

