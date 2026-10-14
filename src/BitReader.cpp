#include <cstdint>
#include <fstream>
#include <cassert>
#include <stdexcept>
#include "huffman/BitReader.hpp"

using namespace std;

BitReader::BitReader(istream &input)
    : inputFile{input}, current_bytes{0}, current_position{0}, valid_bits{0}
{
}

bool BitReader::refill()
{
   char bytes[8];
    inputFile.read(bytes, sizeof(bytes));
    const streamsize bytesRead = inputFile.gcount();

    if (inputFile.bad() || (inputFile.fail() && !inputFile.eof()))
    {
        throw runtime_error("Failed to read from input file");
    }

    current_bytes = 0;
    current_position = 0;
    valid_bits = static_cast<unsigned char>(bytesRead * 8);

    for (streamsize i = 0; i < bytesRead; ++i)
    {
        const uint64_t byte = static_cast<unsigned char>(bytes[i]);
        current_bytes |= byte << (56 - (i * 8));
    }
    return bytesRead > 0;
}

uint64_t BitReader::readBits(uint64_t size)
{
    assert(size <= 64);

    uint64_t result = 0;
    for (uint64_t i = 0; i < size; i++)
    {
        result <<= 1;
        result |= readBit();
    }
    return result;
}

uint8_t BitReader::readBit()
{
    if (current_position == valid_bits && !refill())
    {
        throw runtime_error("Unexpected end of file while reading bits");
    }
    const uint8_t bit = static_cast<uint8_t>(current_bytes >> (63 - current_position)) & 1;
    ++current_position;
    return bit;
}

uint8_t BitReader::readByte()
{
    return static_cast<uint8_t>(readBits(8));
}

BitReader::~BitReader()
{
}