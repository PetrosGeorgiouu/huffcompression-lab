CXX := clang++

# Optimized build settings for compression benchmarks.
CXXFLAGS := -std=c++17 \
            -O2 \
            -g \
            -fno-omit-frame-pointer \
            -DNDEBUG \
            -Wall \
            -Wextra \
            -Wpedantic \
            -Iinclude

SOURCES := src/FrequencyTable.cpp \
           src/HuffmanTree.cpp \
           src/Compression.cpp \
           src/BitWriter.cpp

HEADERS := include/huffman/FrequencyTable.hpp \
           include/huffman/HuffmanTree.hpp \
           include/huffman/Compression.hpp \
           include/huffman/BitWriter.hpp

TESTS := tests/tests.cpp

INPUT ?=
OUTPUT ?=
CORPUS_DIR ?= data/corpus
BENCH_CPU ?= 2
BENCH_ARGS ?= --benchmark_filter='BM_CompressFile.*' \
              --benchmark_repetitions=30

BENCHMARK_INCLUDE := $(HOME)/benchmark/include
BENCHMARK_LIB := $(HOME)/benchmark/build-release/src/libbenchmark.a

build/huff: apps/huff.cpp $(SOURCES) $(HEADERS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) apps/huff.cpp $(SOURCES) \
		-o build/huff


build/huff_profiler: apps/huff_profiler.cpp $(SOURCES) $(HEADERS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) -I$(BENCHMARK_INCLUDE) apps/huff_profiler.cpp $(SOURCES) \
		$(BENCHMARK_LIB) -lpthread \
		-o build/huff_profiler


build/corpus: apps/corpus.cpp $(SOURCES) $(HEADERS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) apps/corpus.cpp $(SOURCES) \
		-o build/corpus


build/tests: $(TESTS) $(SOURCES) $(HEADERS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(TESTS) $(SOURCES) \
		-o build/tests


build: build/huff_profiler build/corpus build/tests

compress: build/huff
	@if [ -z "$(INPUT)" ] || [ -z "$(OUTPUT)" ]; then \
		echo "Usage: make compress INPUT=<input-file> OUTPUT=<output-file>"; \
		exit 1; \
	fi
	./build/huff "$(INPUT)" "$(OUTPUT)"


profile: build/huff_profiler
ifeq ($(shell uname -s),Linux)
	taskset -c $(BENCH_CPU) ./build/huff_profiler $(BENCH_ARGS)
else
	./build/huff_profiler $(BENCH_ARGS)
endif


corpus: build/corpus
	./build/corpus "$(CORPUS_DIR)"


tests: build/tests
	mkdir -p build/test_output
	./build/tests


clean:
	rm -rf build


.PHONY: build compress profile corpus tests clean