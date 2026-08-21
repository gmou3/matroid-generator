CXX := g++
CXXFLAGS := -O3 -march=native -funroll-loops -Wall -Wextra -Wpedantic -Wconversion
SHELL_CMD := bash
SRC_DIR := src
BUILD_DIR := build
TEST_DIR := tests
SZ := $(BUILD_DIR)/sz
IC := $(BUILD_DIR)/IC
IC_EXTEND := $(BUILD_DIR)/IC-extend

SRCS := $(filter-out $(SRC_DIR)/sz.cpp $(SRC_DIR)/IC.cpp $(SRC_DIR)/IC-extend.cpp, $(wildcard $(SRC_DIR)/*.cpp))
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))
IC_OBJ := $(BUILD_DIR)/IC.o
IC_EXTEND_OBJ := $(BUILD_DIR)/IC-extend.o
DEPS := $(OBJS:.o=.d) $(IC_OBJ:.o=.d) $(IC_EXTEND_OBJ:.o=.d) $(BUILD_DIR)/sz.d

all: $(SZ) $(IC) $(IC_EXTEND)

$(SZ): $(SRC_DIR)/sz.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -MF $(BUILD_DIR)/sz.d -o $@ $<

$(IC): $(OBJS) $(IC_OBJ) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -fopenmp -o $@ $^

$(IC_EXTEND): $(OBJS) $(IC_EXTEND_OBJ) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -fopenmp -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -fopenmp -MMD -MP -MF $(BUILD_DIR)/$*.d -c -o $@ $<

-include $(DEPS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

test: all
	$(SHELL_CMD) $(TEST_DIR)/test.sh

format:
	clang-format -i $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/*.h)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all test format clean
