# Makefile for App - OpenGL Cinematic Path Tracer Renderer

BUILD_DIR = build
APP_EXECUTABLE = $(BUILD_DIR)/App/App

.PHONY: all build run clean configure

all: build

configure:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake ..

build: configure
	@cd $(BUILD_DIR) && make -j$(shell sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)

run: build
	@cd $(BUILD_DIR)/App && ./App

clean:
	@rm -rf $(BUILD_DIR)

