# Makefile for App - OpenGL Cinematic Path Tracer Renderer

BUILD_DIR = build
RELEASE_DIR = release
APP_EXECUTABLE = $(BUILD_DIR)/App/App
VERSION ?= 1.0.0

.PHONY: all build run clean configure release package

all: build

configure:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=Release ..

build: configure
	@cd $(BUILD_DIR) && make -j$(shell sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)

run: build
	@cd $(BUILD_DIR)/App && ./App

clean:
	@rm -rf $(BUILD_DIR) $(RELEASE_DIR)

release: build package
	@echo "Release package created in $(RELEASE_DIR)/"

package: build
	@./scripts/package-release.sh $(VERSION)

