#include <iostream>
#include <unordered_map>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <string>
#include <cstdint>

#include "huffman/FrequencyTable.hpp"
#include "huffman/HuffmanTree.hpp"

using namespace std;
using namespace std::chrono;

using Clock = steady_clock;
using Nanoseconds = nanoseconds;

double toSeconds(Nanoseconds duration)
{
     return duration.count() / 1e9;
}

double bytesToMB(uintmax_t bytes)
{
     // Decimal MB: 1 MB = 1,000,000 bytes
     return static_cast<double>(bytes) / 1'000'000.0;
}

double throughputMBPerSecond(double fileMB, Nanoseconds duration)
{
     double seconds = toSeconds(duration);

     if (seconds == 0.0)
     {
          return 0.0;
     }

     return fileMB / seconds;
}

void printStageReport(
    const string &stageName,
    Nanoseconds duration,
    double fileMB)
{
     double seconds = toSeconds(duration);
     double throughput = throughputMBPerSecond(fileMB, duration);

     cout << left << setw(30) << stageName
          << right << setw(12) << fixed << setprecision(6) << seconds << " s"
          << right << setw(18) << fixed << setprecision(3) << throughput << " MB/s"
          << '\n';
}

int main()
{
     // const std::string filePath =
     //     "data/corpus/complete_project_gutenberg_works_of_george_meredith.txt";

     const std::string filePath =
         "tests/huffman_all_256_bytes_stress.txt";

     uintmax_t fileBytes = filesystem::file_size(filePath);
     double fileMB = bytesToMB(fileBytes);

     cout << "================ Huffman Benchmark Report ================\n\n";

     cout << "Input file: " << filePath << '\n';
     cout << "Input size: " << fileBytes << " bytes\n";
     cout << "Input size: " << fixed << setprecision(3) << fileMB << " MB\n\n";

     auto totalStart = Clock::now();

     auto frequencyStart = Clock::now();
     unordered_map<char, u_int64_t> freqs = findFrequenciesNaive(filePath);
     auto frequencyStop = Clock::now();

     auto encodingStart = Clock::now();
     HuffmanTreeNaive huffmanTree(freqs);
     unordered_map<char, string> huffmanCode = huffmanTree.getEncodings();
     auto encodingStop = Clock::now();

     auto totalStop = Clock::now();

     for (auto pair : huffmanCode)
     {
          std::cout << pair.first << ": " << pair.second << "\n";
     }

     auto frequencyDuration =
         duration_cast<Nanoseconds>(frequencyStop - frequencyStart);

     auto encodingDuration =
         duration_cast<Nanoseconds>(encodingStop - encodingStart);

     auto totalDuration =
         duration_cast<Nanoseconds>(totalStop - totalStart);

     cout << "-------------------- Stage Timings ------------------------\n";
     cout << left << setw(30) << "Stage"
          << right << setw(14) << "Time"
          << right << setw(24) << "Throughput"
          << '\n';

     cout << string(68, '-') << '\n';

     printStageReport("Find frequencies", frequencyDuration, fileMB);
     printStageReport("Build tree + encodings", encodingDuration, fileMB);
     printStageReport("Total up to encodings", totalDuration, fileMB);

     cout << "\n-------------------- Summary ------------------------------\n";

     cout << "Unique symbols: " << freqs.size() << '\n';
     cout << "Encodings generated: " << huffmanCode.size() << '\n';

     cout << "Total time: "
          << fixed << setprecision(6)
          << toSeconds(totalDuration)
          << " s\n";

     cout << "End-to-end throughput up to encodings: "
          << fixed << setprecision(3)
          << throughputMBPerSecond(fileMB, totalDuration)
          << " MB/s\n";

     cout << "\nNote: This benchmark currently stops after obtaining Huffman encodings.\n";
     cout << "It does not yet include actual bit-level compression or decompression.\n";

     cout << "\n============================================================\n";

     return 0;
}