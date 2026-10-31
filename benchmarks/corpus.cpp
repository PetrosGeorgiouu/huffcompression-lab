#include "huffman/Compression.hpp"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

int main(int argc, char *argv[])
{
    const fs::path corpusDirectory =
        argc == 2 ? fs::path(argv[1]) : fs::path("data/corpus");

    if (!fs::exists(corpusDirectory))
    {
        std::cerr << "Corpus directory does not exist: "
                  << corpusDirectory << '\n';
        return 1;
    }

    if (!fs::is_directory(corpusDirectory))
    {
        std::cerr << "Corpus path is not a directory: "
                  << corpusDirectory << '\n';
        return 1;
    }

    std::vector<fs::path> inputFiles;

    for (const fs::directory_entry &entry :
         fs::directory_iterator(corpusDirectory))
    {

        if (entry.is_regular_file() &&
            entry.path().extension() == ".txt")
        {
            inputFiles.push_back(entry.path());
        }
    }

    std::sort(inputFiles.begin(), inputFiles.end());

    if (inputFiles.empty())
    {
        std::cerr << "No .txt files found in: "
                  << corpusDirectory << '\n';
        return 1;
    }

    std::uintmax_t totalOriginalBytes = 0;
    std::uintmax_t totalCompressedBytes = 0;

    long double sumOfFileReductionPercentages = 0.0L;

    std::size_t successfulFiles = 0;
    std::size_t failedFiles = 0;

    std::cout << std::left
              << std::setw(32) << "File"
              << std::right
              << std::setw(14) << "Original"
              << std::setw(14) << "Compressed"
              << std::setw(14) << "Saved"
              << std::setw(12) << "Reduction"
              << '\n';

    std::cout << std::string(86, '-') << '\n';

    for (const fs::path &inputPath : inputFiles)
    {
        try
        {
            const std::uintmax_t originalBytes =
                fs::file_size(inputPath);

            fs::path outputPath = inputPath;
            outputPath.replace_extension(".huff");

            compressor(
                inputPath.string(),
                outputPath.string());

            if (!fs::exists(outputPath))
            {
                throw std::runtime_error(
                    "Compressor did not create " +
                    outputPath.string());
            }

            const std::uintmax_t compressedBytes =
                fs::file_size(outputPath);

            const std::intmax_t bytesSaved =
                compressedBytes <= originalBytes
                    ? static_cast<std::intmax_t>(
                          originalBytes - compressedBytes)
                    : -static_cast<std::intmax_t>(
                          compressedBytes - originalBytes);

            const long double reductionPercentage =
                originalBytes == 0
                    ? 0.0L
                    : (1.0L -
                       static_cast<long double>(compressedBytes) /
                           static_cast<long double>(originalBytes)) *
                          100.0L;

            totalOriginalBytes += originalBytes;
            totalCompressedBytes += compressedBytes;
            sumOfFileReductionPercentages += reductionPercentage;
            ++successfulFiles;

            std::cout << std::left
                      << std::setw(32)
                      << inputPath.filename().string()
                      << std::right
                      << std::setw(14) << originalBytes
                      << std::setw(14) << compressedBytes
                      << std::setw(14) << bytesSaved
                      << std::setw(11)
                      << std::fixed << std::setprecision(2)
                      << reductionPercentage
                      << "%\n";
        }
        catch (const std::exception &error)
        {
            ++failedFiles;

            std::cerr << "Failed to benchmark "
                      << inputPath << ": "
                      << error.what() << '\n';
        }
    }

    if (successfulFiles == 0)
    {
        std::cerr << "Every compression attempt failed.\n";
        return 1;
    }

    const std::intmax_t totalBytesSaved =
        totalCompressedBytes <= totalOriginalBytes
            ? static_cast<std::intmax_t>(
                  totalOriginalBytes - totalCompressedBytes)
            : -static_cast<std::intmax_t>(
                  totalCompressedBytes - totalOriginalBytes);

    const long double aggregateReductionPercentage =
        totalOriginalBytes == 0
            ? 0.0L
            : (1.0L -
               static_cast<long double>(totalCompressedBytes) /
                   static_cast<long double>(totalOriginalBytes)) *
                  100.0L;

    const long double averageFileReductionPercentage =
        sumOfFileReductionPercentages /
        static_cast<long double>(successfulFiles);

    const long double averageBytesSavedPerFile =
        static_cast<long double>(totalBytesSaved) /
        static_cast<long double>(successfulFiles);

    std::cout << '\n'
              << "Corpus summary\n"
              << "--------------\n"
              << "Files compressed:          " << successfulFiles << '\n'
              << "Files failed:              " << failedFiles << '\n'
              << "Total original bytes:      " << totalOriginalBytes << '\n'
              << "Total compressed bytes:    " << totalCompressedBytes << '\n'
              << "Total bytes saved:         " << totalBytesSaved << '\n'
              << "Average bytes saved/file:  "
              << std::fixed << std::setprecision(2)
              << averageBytesSavedPerFile << '\n'
              << "Average file reduction:    "
              << averageFileReductionPercentage << "%\n"
              << "Aggregate reduction:       "
              << aggregateReductionPercentage << "%\n";

    return failedFiles == 0 ? 0 : 1;
}