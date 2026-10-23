# Huffman Data Compression Lab

A data compression library for compressing/decompressing text files. Compares a naive implementation to a performance enhanced implementation.

# How to run

Tested on Linux (Ubuntu) and macOS.

### Download

Select **Code → Download ZIP** on this repository's GitHub page and extract the archive, or clone it using its HTTPS URL:

```bash
git clone <repository-url>
```

Open a terminal in the downloaded repository's root directory—the folder containing the `Makefile`.

### Requirements

You need a C++17-capable compiler and `make`.

Google Benchmark is used for performance measurements. Although it is only used for benchmarking, the current Makefile setup requires it to be installed before building.

**Linux (Ubuntu):**

```bash
sudo apt update
sudo apt install build-essential clang libbenchmark-dev
```

**macOS:**

Install Apple's command-line developer tools if they are not already installed:

```bash
xcode-select --install
```

With Homebrew installed, install Google Benchmark:

```bash
brew install google-benchmark
```

If the Makefile cannot locate Google Benchmark, adjust its include and library paths to match your installation. On macOS, find its installation directory with:

```bash
brew --prefix google-benchmark
```

### Build and run

Build the executable and create its output directory:

```bash
make build
```

Compress a text file:

```bash
make compress \
  INPUT="path/to/input.txt" \
  OUTPUT="path/to/output.huff"
```

Replace these paths with your input and output locations.

You can decompress a .huff file with the corresponding command.

```bash
make decompress \
  INPUT="path/to/input.huff" \
  OUTPUT="path/to/output.txt"
```

Run the test cases:

```bash
make tests
```

Measure compression effectiveness across the corpus of 11 text files:

```bash
make corpus
```

Run the performance benchmark:

```bash
make profile
```

This benchmarks the compressor with 30 repetitions using `data/corpus/complete_project_gutenberg_works_of_george_meredith.txt`. Ensure the benchmark input file is available at that path. Results are discussed in the Performance Analysis section below.

Remove the generated build directory and its contents:

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

| Component            |           Value |
| -------------------- | --------------: |
| CPU cores            |               4 |
| CPU frequency        |    ~1996.65 MHz |
| L1 Data Cache        |      48 KiB × 4 |
| L1 Instruction Cache |      32 KiB × 4 |
| L2 Cache             |     512 KiB × 4 |
| L3 Cache             | 6144 KiB shared |

### Baseline Results for Compression

First, let's discuss a constant baseline template for how we will measure if performance actually improved. I use the make command listed above to calculate the latencies on each run.

This is our official, first implementation report on the statistics found.

| Metric                    |           Result |
| ------------------------- | ---------------: |
| Median wall-clock latency |   **722.497 ms** |
| Mean wall-clock latency   |   **725.954 ms** |
| Best observed latency     |   **720.031 ms** |
| Standard deviation        |     **9.894 ms** |
| Coefficient of variation  |        **1.36%** |
| Median CPU time           |   **721.058 ms** |
| Mean CPU time             |   **724.461 ms** |
| Median throughput         | **19.744 MiB/s** |
| Mean throughput           | **19.654 MiB/s** |
| Peak observed throughput  | **19.812 MiB/s** |

> **Note:** The baseline results were remeasured using the updated benchmark methodology—a pinned CPU core, `MinTime(1.0)`, and `30` repetitions—to ensure a stable, apples-to-apples comparison with the optimized implementation.

### Current Results for Compression

We use this to measure the results of statistics based on the most recent change made. I'll include what we changed most recently also.

- **August 16, 2026:** `frequencyCounter()` now returns a `std::array<uint64_t, 256>` instead of an `std::unordered_map<char, uint64_t>`. `getEncodings()` now returns a `std::array<std::string, 256>` instead of an `std::unordered_map<char, std::string>`.
- **August 18, 2026:** `frequencyCounter()` now reads characters from the input file using a `4096`-byte buffer instead of reading one character at a time. The compressor’s encoding pass now also reads the input using a `4096`-byte buffer.
- **September 4, 2026:** `BitWriter` now uses a `4096`-byte output buffer, allowing it to write blocks of bytes to the output file instead of performing a stream write for every individual byte.
- **September 6, 2026:** Each Huffman code is now represented by an `Encoding` struct containing its encoded bits and bit length instead of an `std::string`. The compressor now passes each complete encoding to `BitWriter::writeBits()` instead of calling `BitWriter::writeBit()` separately for every bit.
- **September 11, 2026:** `BitWriter::writeBits()` now stores pending bits in a `64`-bit reservoir and uses a fast path with `64`-bit arithmetic when an incoming Huffman code keeps the total below `64` bits. Otherwise, it delegates to `drainBits()`, which uses a `128`-bit temporary to append the code, emits the oldest `64` bits, and retains the remainder. Marking `drainBits()` as `[[gnu::noinline]]` keeps the draining logic separate; assembly inspection confirmed a tail jump to the helper and removal of four register save/restore pairs from `writeBits()`. This replaces the previous byte-by-byte boundary processing while limiting wide arithmetic to calls that emit an output word. The output buffer was increased from `4 KiB` to `64 KiB`, reducing `writev` calls from `1,047` to `131` on the benchmark workload.
- **October 16, 2026:** Implemented a `UniquePtrPriorityQueue` class to allow for a priority queue to remove `std::unique_ptr` entries via `std::move` operations. Replace raw pointers with `std::unique_ptr` in `HuffmanTree` class and leveraged the `UniquePtrPriorityQueue` class during class construction.

| Metric                    |            Result |
| ------------------------- | ----------------: |
| Median wall-clock latency |     **67.351 ms** |
| Mean wall-clock latency   |     **70.397 ms** |
| Best observed latency     |     **65.345 ms** |
| Standard deviation        |      **8.932 ms** |
| Coefficient of variation  |        **12.69%** |
| Median CPU time           |     **64.800 ms** |
| Mean CPU time             |     **67.396 ms** |
| Median throughput         | **211.804 MiB/s** |
| Mean throughput           | **204.995 MiB/s** |
| Peak observed throughput  | **218.304 MiB/s** |

### Baseline Results for Decompression

Below are the baseline timed results for the initial decompression implementation.

| Metric                    |           Result |
| ------------------------- | ---------------: |
| Median wall-clock latency |   **587.250 ms** |
| Mean wall-clock latency   |   **596.045 ms** |
| Best observed latency     |   **571.618 ms** |
| Standard deviation        |    **23.419 ms** |
| Coefficient of variation  |        **3.93%** |
| Median CPU time           |   **585.103 ms** |
| Mean CPU time             |   **594.400 ms** |
| Median throughput         | **13.9295 MiB/s** |
| Mean throughput           | **13.7434 MiB/s** |
| Peak observed throughput  | **14.3105 MiB/s** |

### Current Results for Decompression

We use this to measure the results of statistics based on the most recent change made. I'll include what we changed most recently also.

| Metric                    |           Result |
| ------------------------- | ---------------: |
| Median wall-clock latency |   **587.250 ms** |
| Mean wall-clock latency   |   **596.045 ms** |
| Best observed latency     |   **571.618 ms** |
| Standard deviation        |    **23.419 ms** |
| Coefficient of variation  |        **3.93%** |
| Median CPU time           |   **585.103 ms** |
| Mean CPU time             |   **594.400 ms** |
| Median throughput         | **13.9295 MiB/s** |
| Mean throughput           | **13.7434 MiB/s** |
| Peak observed throughput  | **14.3105 MiB/s** |

## Acknowledgments

Early versions of my Huffman Coding file were inspired by a C++ implementation done by GeeksforGeeks (https://www.geeksforgeeks.org/cpp/huffman-coding-in-cpp/?_x_tr_hist=true); I also used their tutorial to read from files using C++ (https://www.w3schools.com/cpp/cpp_files.asp).
