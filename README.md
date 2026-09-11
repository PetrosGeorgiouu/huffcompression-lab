# Huffman Data Compression Lab
A data compression library for compressing/decompressing text files. Compares a naive implementation to a performance enhanced implementation.

# How to run

Build the executable and create the directory used to store it:

```bash
make build
```
Here's how to use the compressor.

```bash
make compress \
  INPUT= [input file path] \
  OUTPUT= [output file path]
```
This takes a txt file from the input file path, compresses it, and stores it in the output file path. Note: both must exist for the compressor to succeed.

You can also run its test cases with:

```bash
make tests
```
After finishing the compressor, I used

```bash
make corpus
```
to measure how well our compressor compressed a corpus of 11 text files.

After each implementation improvement, I use

```bash
taskset -c 2 ./build/huff_profiler \
    --benchmark_filter='BM_CompressFile.*' \
    --benchmark_repetitions=30
```
to run the compressor 30 times on "data/corpus/complete_project_gutenberg_works_of_george_meredith.txt" to measure performance. We will discuss this later.

After testing, remove the generated build directory and its contents:

```bash
make clean
```

# Performance Analysis

I'd also like to take some time to explain some of my optimizations and the actual statistics as to how my new model compares to the naive model.

I want to specify the original machine specification I used to measure performance in my original implementation. I used a Ubuntu Linux Virtual Machine.

```text
Environment:
  Ubuntu: Ubuntu 24.04.4 LTS running inside UTM
  Linux kernel: 6.8.0-136-generic
  Architecture: x86_64
  Virtualization: apple, as reported by systemd-detect-virt

CPU:
  Guest-visible CPU model: Intel(R) Core(TM) i5-1038NG7 CPU @ 2.00GHz
  Virtual CPUs: 4
  Physical cores visible to guest: 4
  Threads per core: 1
  L1d cache: 192 KiB (4 instances)
  L1i cache: 128 KiB (4 instances)
  L2 cache: 2 MiB (4 instances)
  L3 cache: 6 MiB (1 instance)

Memory:
  Configured VM memory: 4 GB
  Guest-visible memory: 3.8 GiB

Toolchain:
  Compiler: Ubuntu clang version 18.1.3 (1ubuntu1)
  C++ standard: C++17
  Standard library: libstdc++
  Build mode: Release
  Compiler flags: -std=c++17 -O2 -Wall -Wextra -Wpedantic
```

You can reference this as

| Component | Value |
|---|---:|
| CPU cores | 4 |
| CPU frequency | ~1996.65 MHz |
| L1 Data Cache | 48 KiB × 4 |
| L1 Instruction Cache | 32 KiB × 4 |
| L2 Cache | 512 KiB × 4 |
| L3 Cache | 6144 KiB shared |

### Baseline Results

First, let's discuss a constant baseline template for how we will measure if performance actually improved. I use the make command listed above to calculate the latencies on each run.

This is our official, first implementation report on the statistics found.

| Metric | Result |
|---|---:|
| Median wall-clock latency | **722.497 ms** |
| Mean wall-clock latency | **725.954 ms** |
| Best observed latency | **720.031 ms** |
| Standard deviation | **9.894 ms** |
| Coefficient of variation | **1.36%** |
| Median CPU time | **721.058 ms** |
| Mean CPU time | **724.461 ms** |
| Median throughput | **19.744 MiB/s** |
| Mean throughput | **19.654 MiB/s** |
| Peak observed throughput | **19.812 MiB/s** |

> **Note:** The baseline results were remeasured using the updated benchmark methodology—a pinned CPU core, `MinTime(1.0)`, and `30` repetitions—to ensure a stable, apples-to-apples comparison with the optimized implementation.

### Current Results

We use this to measure the results of statistics based on the most recent change made. I'll include what we changed most recently also.

- **August 16, 2026:** `frequencyCounter()` now returns a `std::array<uint64_t, 256>` instead of an `std::unordered_map<char, uint64_t>`. `getEncodings()` now returns a `std::array<std::string, 256>` instead of an `std::unordered_map<char, std::string>`.
- **August 18, 2026:** `frequencyCounter()` now reads characters from the input file using a `4096`-byte buffer instead of reading one character at a time. The compressor’s encoding pass now also reads the input using a `4096`-byte buffer.
- **September 4, 2026:** `BitWriter` now uses a `4096`-byte output buffer, allowing it to write blocks of bytes to the output file instead of performing a stream write for every individual byte.
- **September 6, 2026:** Each Huffman code is now represented by an `Encoding` struct containing its encoded bits and bit length instead of an `std::string`. The compressor now passes each complete encoding to `BitWriter::writeBits()` instead of calling `BitWriter::writeBit()` separately for every bit.
- **September 11, 2026:** `BitWriter::writeBits()` now stores pending bits in a `64`-bit reservoir, uses a `128`-bit temporary to append complete Huffman codes, and drains completed `64`-bit words into its output buffer. This replaces the previous byte-by-byte boundary processing while avoiding persistent `128`-bit state. The output buffer was increased from `4 KiB` to `64 KiB`, reducing `writev` calls from `1,047` to `131` on the benchmark workload.

| Metric | Result |
|---|---:|
| Median wall-clock latency | **93.767 ms** |
| Mean wall-clock latency | **94.231 ms** |
| Best observed latency | **92.109 ms** |
| Standard deviation | **1.346 ms** |
| Coefficient of variation | **1.43%** |
| Median CPU time | **92.025 ms** |
| Mean CPU time | **92.382 ms** |
| Median throughput | **152.134 MiB/s** |
| Mean throughput | **151.414 MiB/s** |
| Peak observed throughput | **154.872 MiB/s** |

## Acknowledgments

Early versions of my Huffman Coding file were inspired by a C++ implementation done by GeeksforGeeks (https://www.geeksforgeeks.org/cpp/huffman-coding-in-cpp/?_x_tr_hist=true); I also used their tutorial to read from files using C++ (https://www.w3schools.com/cpp/cpp_files.asp).
