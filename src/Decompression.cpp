#include <istream>
#include <ostream>
#include <fstream>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include "huffman/BitReader.hpp"

namespace fs = std::filesystem;
using namespace std;

void decompressor(const std::string &inputPath, const std::string &outputPath)
{
    ofstream outputFile(outputPath);

    if (!outputFile)
    {
        throw runtime_error(
            "Failed to open output file: " + outputPath);
    }

    ifstream file(inputPath, ios::binary);

    // confirm it's a HUFF file
    assert(file.get() == 'H');
    assert(file.get() == 'U');
    assert(file.get() == 'F');
    assert(file.get() == 'F');

    // confirm version is 1
    const std::uint8_t version = file.get();
    assert(version == 1);

    // read original size in bytes
    uint64_t original_size;
    file.read(reinterpret_cast<char *>(&original_size), sizeof(original_size));

    // read serialized tree
}