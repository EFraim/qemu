# Makefile for ARM32->ARM64 Fast Transpiler PoC
# 
# This standalone Makefile allows building and testing the transpiler
# independently of full QEMU integration.

CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -g -fPIC
LDFLAGS ?=

# Source files
SOURCES = arm32-decoder.c arm64-emitter.c arm32-transpiler.c
HEADERS = arm32-decoder.h arm64-emitter.h arm32-transpiler.h

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Targets
LIBRARY = libarm32transpiler.a
TEST_BINARY = test-transpiler
TEST_SOURCE = test-transpiler.c

# Default target
all: $(LIBRARY) $(TEST_BINARY)

# ============================================================================
# Static Library
# ============================================================================

$(LIBRARY): $(OBJECTS)
	ar rcs $@ $^
	@echo "✓ Built static library: $@"

# ============================================================================
# Test Binary
# ============================================================================

$(TEST_BINARY): $(OBJECTS) test-transpiler.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✓ Built test binary: $@"

# ============================================================================
# Object Files
# ============================================================================

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

test-transpiler.o: $(TEST_SOURCE) $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# ============================================================================
# Test Execution
# ============================================================================

test: $(TEST_BINARY)
	@echo "Running transpiler tests..."
	@./$(TEST_BINARY)

# ============================================================================
# Cleanup
# ============================================================================

clean:
	rm -f $(OBJECTS) test-transpiler.o
	rm -f $(LIBRARY) $(TEST_BINARY)
	@echo "✓ Cleaned build artifacts"

distclean: clean
	rm -f *~
	@echo "✓ Cleaned all generated files"

# ============================================================================
# Development Targets
# ============================================================================

# Verbose compile (show all commands)
verbose: CFLAGS += -v
verbose: clean all

# Compile with additional debugging flags
debug: CFLAGS = -Wall -Wextra -O0 -g3 -fPIC -DDEBUG
debug: clean all
	@echo "✓ Built with debug symbols"

# Static analysis with gcc
analyze:
	@echo "Running static analysis..."
	gcc --analyze $(SOURCES) $(TEST_SOURCE)

# Show build info
info:
	@echo "ARM32->ARM64 Transpiler PoC - Build Information"
	@echo "================================================"
	@echo "CC: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo "Sources: $(SOURCES)"
	@echo "Headers: $(HEADERS)"
	@echo "Test: $(TEST_SOURCE)"

# ============================================================================
# Phony Targets
# ============================================================================

.PHONY: all test clean distclean verbose debug analyze info

# ============================================================================
# Help
# ============================================================================

help:
	@echo "ARM32->ARM64 Fast Transpiler - Available Targets"
	@echo "=================================================="
	@echo "  all        - Build library and test binary (default)"
	@echo "  test       - Run test suite"
	@echo "  clean      - Remove build artifacts"
	@echo "  distclean  - Remove all generated files"
	@echo "  verbose    - Rebuild with verbose output"
	@echo "  debug      - Build with debugging symbols"
	@echo "  analyze    - Run static analysis"
	@echo "  info       - Show build configuration"
	@echo "  help       - Display this help message"
