#include <algorithm>
#include <array>
#include <cstddef>
#include <istream>
#include <ostream>
#include <fstream>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include "huffman/BitReader.hpp"
#include "huffman/HuffmanDecoder.hpp"

namespace fs = std::filesystem;
using namespace std;

void decompressor(const std::string &inputPath, const std::string &outputPath)
{
    ofstream outputFile(outputPath, ios::binary | ios::trunc);

    if (!outputFile)
    {
        throw runtime_error(
            "Failed to open output file: " + outputPath);
    }

    ifstream file(inputPath, ios::binary);
    if (!file)
    {
        throw runtime_error("Failed to open input file: " + inputPath);
    }

    // confirm it's a HUFF file
    char magic[4];
    if (!file.read(magic, sizeof(magic)) ||
        std::string(magic, sizeof(magic)) != "HUFF")
    {
        throw runtime_error("Invalid Huffman file header");
    }

    // confirm version number 1

    if (file.get() != 1)
    {
        throw runtime_error("Unsupported or missing Huffman version");
    }

    // read original size in bytes
    uint64_t original_size;
    file.read(reinterpret_cast<char *>(&original_size), sizeof(original_size));

    if (!file)
    {
        throw runtime_error("Failed to read Huffman header");
    }

    // Empty input has no serialized tree.
    if (original_size == 0)
    {
        return;
    }

    BitReader reader(file);
    HuffmanDecoder tree(reader);

    std::array<char, 64 * 1024> outputBuffer;
    uint64_t remaining = original_size;

    while (remaining != 0)
    {
        const std::size_t count = static_cast<std::size_t>(
            std::min<uint64_t>(remaining, outputBuffer.size()));

        tree.decodeBlock(outputBuffer.data(), count);
        outputFile.write(outputBuffer.data(),
                         static_cast<std::streamsize>(count));

        if (!outputFile)
        {
            throw runtime_error("Failed to write decompressed output");
        }

        remaining -= count;
    }

    outputFile.flush();
    if (!outputFile)
    {
        throw runtime_error("Failed to write decompressed output");
    }
}
