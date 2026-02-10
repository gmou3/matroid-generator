CXX := g++
CXXFLAGS := -O2 -march=native -fopenmp -Wall -Wextra -Wpedantic -Wconversion
LDFLAGS := -llzma

SHELL_CMD := bash

SRC_DIR := src
BUILD_DIR := build
TEST_DIR := tests
TARGET := $(BUILD_DIR)/IC

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
HEADERS := $(wildcard $(SRC_DIR)/*.h)

all: $(TARGET)

$(TARGET): $(SRCS) $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

test: $(TARGET)
	$(SHELL_CMD) $(TEST_DIR)/test.sh

format:
	clang-format -i $(SRCS) $(HEADERS)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all test format clean
