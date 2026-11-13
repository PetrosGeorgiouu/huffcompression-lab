#include "huffman/FrequencyTable.hpp"

#include <array>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>

std::array<std::uint64_t, 256> findFrequencies(
    const std::string &txtPath)
{
    std::ifstream file(txtPath, std::ios::binary);

    if (!file)
    {
        throw std::runtime_error(
            "Failed to open input file: " + txtPath);
    }

    std::array<std::uint64_t, 256> freqs{};
    char buffer[4096];

    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0)
    {
        const std::streamsize count = file.gcount();

        for (std::streamsize i = 0; i < count; ++i)
        {
            const auto c = static_cast<unsigned char>(buffer[i]);
            ++freqs[c];
        }
    }

    if (file.bad() || (file.fail() && !file.eof()))
    {
        throw std::runtime_error(
            "Failed while reading input file: " + txtPath);
    }

    return freqs;
}