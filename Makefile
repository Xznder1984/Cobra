.PHONY: all build install clean test benchmark docs run-compiler run

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lm -lpthread

COMPILER_DIR = compiler
CLI_DIR = cli
RUNTIME_DIR = runtime

all: build

build:
	@echo "Building Cobra compiler..."
	$(MAKE) -C $(COMPILER_DIR)
	@echo "Building Cobra CLI..."
	$(MAKE) -C $(CLI_DIR)
	@echo "Building Cobra runtime..."
	$(MAKE) -C $(RUNTIME_DIR)
	@echo "Build complete."

install: build
	@echo "Installing Cobra..."
	cp cli/bin/cobra /usr/local/bin/
	cp compiler/bin/cobrac /usr/local/bin/
	cp runtime/libcobra_runtime.a /usr/local/lib/
	mkdir -p /usr/local/lib/cobra
	cp -r stdlib/* /usr/local/lib/cobra/
	@echo "Installation complete."

test:
	@echo "Running tests..."
	$(MAKE) -C tests test

benchmark:
	@echo "Running benchmarks..."
	$(MAKE) -C benchmarks benchmark

clean:
	$(MAKE) -C $(COMPILER_DIR) clean
	$(MAKE) -C $(CLI_DIR) clean
	$(MAKE) -C $(RUNTIME_DIR) clean
	rm -rf examples/*/build
	rm -rf tests/build

docs:
	@echo "Generating documentation..."
	mkdir -p docs/generated
	./cli/bin/cobra docs -o docs/generated

run-compiler:
	./compiler/bin/cobrac
