#include "huffman/Compression.hpp"
#include "huffman/BitWriter.hpp"
#include "huffman/FrequencyTable.hpp"
#include "huffman/HuffmanTree.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

void compressor(const std::string &inputPath, const std::string &outputPath)
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

     // Find character frequencies and construct the Huffman encodings.
    std::array<std::uint64_t, 256> freqs =
    findFrequencies(inputPath);
    HuffmanTree tree(freqs);
    std::array<HuffmanTree::Encoding, 256> encodings =
    tree.getEncodings();
    // Only create or truncate output after the scan succeeds.
    std::ofstream outputFile(
        outputPath, std::ios::binary | std::ios::trunc);

    if (!outputFile)
    {
        throw std::runtime_error(
            "Failed to open output file: " + outputPath);
    }

    BitWriter writer(outputFile);

    // Write magic bytes, version, original size, and serialized tree.
    outputFile.write("HUFF", 4);

    const std::uint8_t version = 1;
    outputFile.write(
        reinterpret_cast<const char *>(&version), sizeof(version));

    std::uint64_t total = 0;
    for (int i = 0; i < 256; ++i)
    {
        total += freqs[i];
    }

    outputFile.write(
        reinterpret_cast<const char *>(&total), sizeof(total));

    tree.serialize(writer);

    if (!outputFile)
    {
        throw std::runtime_error(
            "Failed while writing Huffman header");
    }

    char buffer[4096];

    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0)
    {
        std::streamsize count = file.gcount();

        for (std::streamsize i = 0; i < count; ++i)
        {
            char c = buffer[i];

            const HuffmanTree::Encoding &encoded =
                encodings[static_cast<unsigned char>(c)];

            writer.writeBits(encoded.encoding, encoded.size);
        }
    }

    if (file.bad() || (file.fail() && !file.eof()))
    {
        throw std::runtime_error(
        "Failed while reading input file: " + inputPath);
    }

    writer.flush();
    outputFile.flush();

    if (!outputFile)
    {
        throw std::runtime_error(
            "Failed while writing output file: " + outputPath);
    }

    outputFile.close();

    if (outputFile.fail())
    {
        throw std::runtime_error(
            "Failed to close output file: " + outputPath);
    }
}