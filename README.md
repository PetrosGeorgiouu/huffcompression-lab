# Huffman Data Compression Lab

A C++17 project for lossless Huffman compression and decompression, with tests, corpus measurements, and performance benchmarks. The project explores how lookup tables, buffering, data layout, and assembly inspection improve end-to-end file processing.

On the recorded workloads, optimized compression reduced median latency from **722.497 ms to 77.837 ms**, while optimized decompression reduced it from **653.441 ms to 112.167 ms**.

## Getting started

Tested on Ubuntu Linux and macOS. Run the commands below in a terminal from the repository root, which contains the `Makefile`.

### Download

```bash
git clone https://github.com/PetrosGeorgiouu/huffcompression-lab.git
cd huffcompression-lab
```

Alternatively, select **Code → Download ZIP** on GitHub, extract the archive, and open a terminal in the extracted directory.

### Requirements

The project uses C++17 with Clang/GCC extensions, including `unsigned __int128`. The Makefile uses `clang++` by default.

Google Benchmark is required for the benchmark executables and `make build`. Compression, decompression, tests, and corpus measurements can be built independently without it.

**Ubuntu:**

```bash
sudo apt update
sudo apt install build-essential clang libbenchmark-dev
```

The Makefile's default Linux benchmark paths point to a source installation under `~/benchmark`. When using Ubuntu's `libbenchmark-dev` package, provide the overrides shown in the benchmark commands below.

**macOS:**

Install Apple's command-line developer tools if needed:

```bash
xcode-select --install
```

With Homebrew installed:

```bash
brew install google-benchmark
```

The Makefile locates Google Benchmark through Homebrew. To inspect its installation path:

```bash
brew --prefix google-benchmark
```

### Compress and decompress

Compress a file:

```bash
make compress \
  INPUT="path/to/input.txt" \
  OUTPUT="path/to/output.huff"
```

Decompress an archive:

```bash
make decompress \
  INPUT="path/to/input.huff" \
  OUTPUT="path/to/output.txt"
```

Replace the example paths with your own. These targets build their corresponding executable automatically. Input and output must refer to different files.

### Tests and memory checks

Run the tests:

```bash
make tests
```

The tests cover bit reading and writing, Huffman decoding, round trips, malformed inputs, and file error handling.

On Linux, install Valgrind and run the memory check:

```bash
sudo apt install valgrind
make leak-check
cat build/leak-report.txt
```

The `leak-check` target is configured for Linux.

## Benchmarks

### Build benchmark executables

On Ubuntu with `libbenchmark-dev`:

```bash
make build BENCHMARK_INCLUDE=/usr/include BENCHMARK_LIB=-lbenchmark
```

On macOS, or Linux with the expected source installation under `~/benchmark`:

```bash
make build
```

This target builds both performance benchmark executables, the corpus tool, and the test executable. It does not run them.

### Fixed workloads

Compression uses the collected works of George Meredith from Project Gutenberg:

```text
data/corpus/complete_project_gutenberg_works_of_george_meredith.txt
```

The historical decompression comparisons use a frozen copy of the original Meredith archive:

```text
data/benchmarks/meredith_legacy.huff
```

For those comparisons, the `inputPath` in `benchmarks/dehuff_profiler.cpp` must be:

```cpp
const std::string inputPath =
    "data/benchmarks/meredith_legacy.huff";
```

Keep this archive unchanged. Regenerating a compressed file can change its size and encoding distribution, which changes the decompression workload. A newly compressed Meredith archive can be benchmarked separately, but its timings should not be mixed with the historical results below.

Ensure the required input files are available before running the benchmarks.

### Run timing benchmarks

On Ubuntu with `libbenchmark-dev`:

```bash
make profile BENCHMARK_INCLUDE=/usr/include BENCHMARK_LIB=-lbenchmark
make profile-decompress BENCHMARK_INCLUDE=/usr/include BENCHMARK_LIB=-lbenchmark
```

On macOS, or Linux with the expected source installation:

```bash
make profile
make profile-decompress
```

Each command runs **30 benchmark repetitions** with a minimum measurement time of one second per repetition. Google Benchmark chooses the number of file-processing iterations within each repetition; these commands do not perform just 30 individual file operations.

On Linux, the targets pin execution to CPU 2 using `taskset`. Override this with an available CPU index if needed, for example:

```bash
make profile-decompress BENCH_CPU=0 BENCHMARK_INCLUDE=/usr/include BENCHMARK_LIB=-lbenchmark
```

These measurements time whole-file compression or decompression, including file I/O and codec setup. They measure repeated file-processing performance rather than cold-storage latency or per-packet tail latency.

### Corpus compression ratio

Measure compression effectiveness across the 11 text files:

```bash
make corpus
```

This command compresses every `.txt` file in `data/corpus`, overwriting its corresponding `.huff` file, and reports per-file and aggregate size reductions. To use another directory:

```bash
make corpus CORPUS_DIR="path/to/corpus"
```

The current corpus achieved an aggregate size reduction of approximately **41.6%**. All 11 regenerated archives were decompressed and verified to match their source files byte for byte.

The earlier **43.2%** figure came from older archives whose decoded contents omitted non-ASCII bytes present in the current source files. It is not a valid lossless compression ratio for the current corpus. The frozen legacy archive remains useful as a consistent historical performance workload.

### Clean generated build files

```bash
make clean
```

This removes the `build` directory. It does not remove corpus archives.

## Performance analysis

### Measurement environment

The recorded Linux measurements were collected in an Ubuntu virtual machine:

| Component | Configuration |
| --- | --- |
| Operating system | Ubuntu 24.04.4 LTS inside UTM |
| Kernel | Linux 6.8.0-136-generic |
| Architecture | x86_64 |
| Virtualization | `apple`, as reported by `systemd-detect-virt` |
| Guest-visible CPU | Intel Core i5-1038NG7 at approximately 2.00 GHz |
| Virtual CPUs | 4, with one thread per guest-visible core |
| L1 data cache | 48 KiB × 4 |
| L1 instruction cache | 32 KiB × 4 |
| L2 cache | 512 KiB × 4 |
| L3 cache | 6 MiB shared |
| Memory | 4 GB configured; approximately 3.8 GiB visible |
| Compiler | Ubuntu Clang 18.1.3 |
| Standard library | libstdc++ |

The current benchmark build uses:

```text
-std=c++17 -O2 -g -fno-omit-frame-pointer -DNDEBUG -Wall -Wextra -Wpedantic
```

The baseline measurements were repeated using the pinned-core, one-second minimum measurement time, and 30-repetition methodology used for the optimized comparisons.

In the tables, standard deviation and coefficient of variation refer to wall-clock latency across repetitions. Compression throughput counts **uncompressed input bytes**; decompression throughput counts **compressed input bytes**. Their MiB/s values therefore use different byte counts and should not be compared directly.

### Compression baseline

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

### Optimized compression

- **August 16, 2026:** Replaced hash maps for byte frequencies and encoding lookup with fixed arrays containing 256 entries.
- **August 18, 2026:** Added 4 KiB input buffers to both frequency counting and the compression pass, replacing character-at-a-time reads.
- **September 4, 2026:** Added a 4 KiB output buffer to `BitWriter`, replacing individual byte writes with block writes.
- **September 6, 2026:** Replaced string-based Huffman codes with an `Encoding` struct containing packed bits and a bit length. The compressor passes each complete code to `writeBits()` instead of calling `writeBit()` for each bit.
- **September 11, 2026:** Added a 64-bit bit reservoir with a fast path for codes that keep the pending total below 64 bits. A separate `drainBits()` helper uses a 128-bit temporary when emitting an output word. Assembly inspection confirmed that marking the helper `[[gnu::noinline]]` enabled a tail jump and removed four register save/restore pairs from `writeBits()`. Increasing the output buffer to 64 KiB reduced `writev` calls from 1,047 to 131 on the benchmark workload.
- **October 16, 2026:** Replaced raw tree ownership with `std::unique_ptr` and introduced `UniquePtrPriorityQueue` to support moving owned nodes out of the queue during tree construction.

Latest optimized measurements, recorded November 13, 2026:

| Metric                    |            Result |
| ------------------------- | ----------------: |
| Median wall-clock latency |     **77.837 ms** |
| Mean wall-clock latency   |     **78.625 ms** |
| Best observed latency     |     **76.693 ms** |
| Standard deviation        |      **2.410 ms** |
| Coefficient of variation  |         **3.07%** |
| Median CPU time           |     **77.836 ms** |
| Mean CPU time             |     **78.551 ms** |
| Median throughput         | **183.269 MiB/s** |
| Mean throughput           | **181.583 MiB/s** |
| Peak observed throughput  | **186.003 MiB/s** |

Compared with the recorded baseline, median compression latency decreased by **89.23%**, a **9.28× speedup** on this workload.

### Decompression baseline

| Metric                    |           Result |
| ------------------------- | ---------------: |
| Median wall-clock latency |   **653.441 ms** |
| Mean wall-clock latency   |   **655.736 ms** |
| Best observed latency     |   **647.906 ms** |
| Standard deviation        |    **10.302 ms** |
| Coefficient of variation  |        **1.57%** |
| Median CPU time           |   **651.208 ms** |
| Mean CPU time             |   **653.869 ms** |
| Median throughput         | **12.5185 MiB/s** |
| Mean throughput           | **12.4775 MiB/s** |
| Peak observed throughput  | **12.6255 MiB/s** |

### Optimized decompression

- **October 31, 2026:** Replaced per-byte `ostream::put()` calls with a 64 KiB output buffer. Decoded bytes accumulate in an array and are written using `ostream::write()` when the buffer fills, with any remaining bytes written after decoding.
- **November 13, 2026:** Added a 1,024-entry lookup table for codes up to 10 bits long. The decoder peeks at upcoming bits, looks up the symbol and code length, then consumes only the matched bits. Longer codes and insufficient lookahead near EOF fall back to tree traversal. Refill logic preserves unread bits, and header-defined `tryPeekBits()` and `consumeBits()` allow inlining. A 64 KiB input buffer reduces stream calls. Changing `Entry::bitsConsumed` from `uint64_t` to `uint8_t` reduced each entry from 16 bytes to 2 bytes on the measured platform, shrinking the table from 16 KiB to 2 KiB. A bulk-load refill path replaces byte-by-byte reservoir assembly where enough buffered input is available. Finally, `decodeBlock()` decodes many symbols per call, reducing repeated function calls and setup.

| Metric                    |           Result |
| ------------------------- | ---------------: |
| Median wall-clock latency |   **112.167 ms** |
| Mean wall-clock latency   |   **113.039 ms** |
| Best observed latency     |   **110.771 ms** |
| Standard deviation        |     **2.661 ms** |
| Coefficient of variation  |        **2.35%** |
| Median CPU time           |   **110.460 ms** |
| Mean CPU time             |   **110.844 ms** |
| Median throughput         | **72.9281 MiB/s** |
| Mean throughput           | **72.4027 MiB/s** |
| Peak observed throughput  | **73.8474 MiB/s** |

Compared with the recorded baseline, median decompression latency decreased by **82.83%**, a **5.83× speedup** on the frozen workload.

## Acknowledgments

Early versions of the Huffman implementation were inspired by [GeeksforGeeks' Huffman coding example](https://www.geeksforgeeks.org/cpp/huffman-coding-in-cpp/). [W3Schools' C++ files tutorial](https://www.w3schools.com/cpp/cpp_files.asp) provided an introduction to file handling.