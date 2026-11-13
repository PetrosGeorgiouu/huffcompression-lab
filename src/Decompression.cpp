#include "huffman/Decompression.hpp"
#include "huffman/BitReader.hpp"
#include "huffman/HuffmanDecoder.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

void decompressor(
    const std::string &inputPath,
    const std::string &outputPath)
{
    std::ifstream file(inputPath, std::ios::binary);

    if (!file)
    {
        throw std::runtime_error(
            "Failed to open input file: " + inputPath);
    }

    if (std::filesystem::exists(outputPath) &&
        std::filesystem::equivalent(inputPath, outputPath))
    {
        throw std::runtime_error(
            "Input and output refer to the same file");
    }

    char magic[4];

    if (!file.read(magic, sizeof(magic)) ||
        std::string(magic, sizeof(magic)) != "HUFF")
    {
        throw std::runtime_error("Invalid Huffman file header");
    }

    if (file.get() != 1)
    {
        throw std::runtime_error(
            "Unsupported or missing Huffman version");
    }

    std::uint64_t original_size;
    file.read(
        reinterpret_cast<char *>(&original_size),
        sizeof(original_size));

    if (!file)
    {
        throw std::runtime_error("Failed to read Huffman header");
    }

    std::ofstream outputFile(
        outputPath, std::ios::binary | std::ios::trunc);

    if (!outputFile)
    {
        throw std::runtime_error(
            "Failed to open output file: " + outputPath);
    }

    // Empty input has no serialized tree.
    if (original_size == 0)
    {
        return;
    }

    BitReader reader(file);
    HuffmanDecoder tree(reader);

    std::array<char, 64 * 1024> outputBuffer;
    std::uint64_t remaining = original_size;

    while (remaining != 0)
    {
        const std::size_t count = static_cast<std::size_t>(
            std::min<std::uint64_t>(
                remaining, outputBuffer.size()));

        tree.decodeBlock(outputBuffer.data(), count);

        outputFile.write(
            outputBuffer.data(),
            static_cast<std::streamsize>(count));

        if (!outputFile)
        {
            throw std::runtime_error(
                "Failed to write decompressed output");
        }

        remaining -= count;
    }

    outputFile.flush();

    if (!outputFile)
    {
        throw std::runtime_error(
            "Failed to write decompressed output");
    }
}