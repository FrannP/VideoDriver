CXX      := g++
# Base flags for both modes
COMMON_FLAGS := -Iinclude -Wall -Wextra -std=c++23
LDFLAGS      := 

# Directories
SRC_DIR := src
INC_DIR := include
OBJ_DIR := obj

# Files
SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS := $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
TARGET  := video_app

# --- Target Specific Configuration ---

# Default to release if just 'make' is typed
all: release

# Debug Target: Adds the DEBUG macro and debug symbols for GDB
debug: CXXFLAGS := $(COMMON_FLAGS) -DDEBUG -g -O0
debug: $(TARGET)

# Release Target: Adds high optimization and removes debug overhead
release: CXXFLAGS := $(COMMON_FLAGS) -O3
release: $(TARGET)

# --- Build Rules ---

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean debug release