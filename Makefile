# ======================================================================
# Root Makefile: Builds all three subprojects in the correct order.
# Usage:
#	make              build everything
#   make tetris       build corestack + tetris only
#   make bomberman    build corestack + bomberman only
#   make test         run all tests across all projects
#   make clean        wipe all build artifacts
# ======================================================================

.PHONY: all corestack tetris bomberman test clean depclean

all: corestack tetris bomberman

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