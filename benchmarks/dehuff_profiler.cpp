#include <benchmark/benchmark.h>

#include "huffman/Decompression.hpp"

#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>

// ============================================================
// Golden workload
//
// This is our fixed baseline benchmark.
// Do not change this benchmark while comparing optimizations.
// ============================================================

static void BM_DecompressFile(benchmark::State &state)
{
  const std::string inputPath =
      "data/corpus/complete_project_gutenberg_works_of_george_meredith.huff";

  const std::string outputPath =
      "build/benchmark_output.txt";

  const auto inputBytes =
      std::filesystem::file_size(inputPath);

  for (auto _ : state)
  {
    decompressor(inputPath, outputPath);
  }

  state.SetBytesProcessed(
      static_cast<std::int64_t>(state.iterations() * inputBytes));
}

BENCHMARK(BM_DecompressFile)
    ->UseRealTime()
    ->MinTime(1.0);

// ============================================================
// Google Benchmark entry point
// ============================================================

BENCHMARK_MAIN();