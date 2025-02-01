# Compiler and flags
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Werror -pedantic -g -Iinclude
LDFLAGS = -Lbuild/lib -lbignum

# Directories
SRC_DIR = src/c
INC_DIR = include
BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin
LIB_DIR = $(BUILD_DIR)/lib

# Files
SRC = \
  $(SRC_DIR)/bignum.c \
  $(SRC_DIR)/tfm_opencl.c
TEST_SRC = \
  tests/c/test_bignum.c \
  tests/c/test_tfm.c \
  tests/c/test_mandelbrot_tfm.c
OBJ = $(SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TEST_OBJ = $(TEST_SRC:tests/c/%.c=$(BUILD_DIR)/%.o)
LIB = $(LIB_DIR)/libbignum.so
TEST_EXE = $(BIN_DIR)/test_bignum

# Targets
all: $(BIN_DIR) $(LIB_DIR) $(LIB) $(TEST_EXE)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(BUILD_DIR)/%.o: tests/c/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB): $(OBJ)
	$(CC) -shared -rdynamic -o $@ $(OBJ)

$(TEST_EXE): $(TEST_OBJ)
	$(CC) $(CFLAGS)  $(TEST_OBJ) -o $@ $(LDFLAGS)

# Clean up
clean:
	rm -rf $(BUILD_DIR)

# Phony targets
.PHONY: all clean

# debug
debug:
	@echo "TEST_OBJ is $(TEST_OBJ)"
