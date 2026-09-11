#include <benchmark/benchmark.h>

#include "huffman/Compression.hpp"

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

static void BM_CompressFile(benchmark::State& state)
{
    const std::string inputPath =
        "data/corpus/complete_project_gutenberg_works_of_george_meredith.txt";

    const std::string outputPath =
        "build/benchmark_output.huff";

    const auto inputBytes =
        std::filesystem::file_size(inputPath);

    for (auto _ : state)
    {
        compressor(inputPath, outputPath);
    }

    state.SetBytesProcessed(
        static_cast<std::int64_t>(state.iterations() * inputBytes));
}


BENCHMARK(BM_CompressFile)
    ->UseRealTime()
    ->MinTime(1.0);


// ============================================================
// Scaling workloads
//
// These deliberately use prefixes of the same source file so
// that input size changes while workload composition remains
// reasonably similar.
// ============================================================

static std::string scaledInputPath(std::int64_t mib)
{
    switch (mib)
    {
        case 1:
            return "data/perf/meredith_1MiB.txt";

        case 2:
            return "data/perf/meredith_2MiB.txt";

        case 4:
            return "data/perf/meredith_4MiB.txt";

        case 8:
            return "data/perf/meredith_8MiB.txt";

        default:
            throw std::runtime_error(
                "Unsupported scaling benchmark size");
    }
}


static void BM_CompressScale(benchmark::State& state)
{
    const std::int64_t mib = state.range(0);

    const std::string inputPath =
        scaledInputPath(mib);

    const std::string outputPath =
        "build/benchmark_scale_output.huff";

    const auto inputBytes =
        std::filesystem::file_size(inputPath);

    for (auto _ : state)
    {
        compressor(inputPath, outputPath);
    }

    state.SetBytesProcessed(
        static_cast<std::int64_t>(state.iterations() * inputBytes));
}


BENCHMARK(BM_CompressScale)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8)
    ->UseRealTime()
    ->MinTime(1.0);


// ============================================================
// Google Benchmark entry point
// ============================================================

BENCHMARK_MAIN();