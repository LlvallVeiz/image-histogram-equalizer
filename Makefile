# Makefile -- image-histogram-equalizer
# C++17, no external dependencies. `make test` builds and runs the
# assert-based unit test suite; `make examples` regenerates the example
# images and their HE/CLAHE outputs under examples/.

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc

SRC      := $(wildcard src/*.cpp)
LIB_SRC  := $(filter-out src/main.cpp,$(SRC))
TEST_SRC := $(wildcard tests/*.cpp)

TARGET      := ihe
TEST_TARGET := run_tests

.PHONY: all clean test examples

all: $(TARGET)

$(TARGET): $(SRC) $(wildcard src/*.h)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

$(TEST_TARGET): $(LIB_SRC) $(TEST_SRC) $(wildcard src/*.h tests/*.h)
	$(CXX) $(CXXFLAGS) $(LIB_SRC) $(TEST_SRC) -o $(TEST_TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

examples: $(TARGET)
	bash examples/run_examples.sh

clean:
	rm -f $(TARGET) $(TEST_TARGET)
