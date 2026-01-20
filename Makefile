CXX := g++
CXXFLAGS := -fopenmp -O3
SHELL_CMD := bash

SRC_DIR := src
BUILD_DIR := build
TEST_DIR := tests
TARGET := $(BUILD_DIR)/IC

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
HEADERS := $(wildcard $(SRC_DIR)/*.h)

all: $(TARGET)

$(TARGET): $(SRCS) $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

test: $(TARGET)
	$(SHELL_CMD) $(TEST_DIR)/test.sh

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all test clean
