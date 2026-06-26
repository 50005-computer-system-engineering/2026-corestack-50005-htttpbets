# Makefile for libhtttp, AI generated

CC      := gcc
CFLAGS  := -Wall -Wextra -Wpedantic -std=c11 -Iinclude
ARFLAGS := rcs

SRC_DIR := src
OBJ_DIR := build/obj
LIB_DIR := build/lib

# Collect all .c sources recursively under src/
SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

TARGET_STATIC  := $(LIB_DIR)/libhtttp.a
TARGET_SHARED  := $(LIB_DIR)/libhtttp.so

TEST_SRC := tests/test_libhtttp.c
TEST_BIN := build/test_libhtttp

.PHONY: all static shared test clean

all: static shared

# --- Static library ---
static: $(TARGET_STATIC)

$(TARGET_STATIC): $(OBJS) | $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^
	@echo "Built static library: $@"

# --- Shared library ---
shared: CFLAGS += -fPIC
shared: $(TARGET_SHARED)

$(TARGET_SHARED): $(OBJS) | $(LIB_DIR)
	$(CC) -shared -o $@ $^
	@echo "Built shared library: $@"

# --- Compile object files ---
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# --- Create output directories ---
build $(OBJ_DIR) $(LIB_DIR):
	mkdir -p $@

# --- Test ---
test: $(TEST_BIN)
	@echo "Running tests..."
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRC) $(TARGET_STATIC) | build
	$(CC) $(CFLAGS) $< -o $@ -Lbuild/lib -lhtttp
	@echo "Built test binary: $@"

# --- Clean ---
clean:
	rm -rf build/
	@echo "Cleaned build artifacts."
