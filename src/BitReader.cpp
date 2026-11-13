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
    const unsigned remaining =
        valid_bits - static_cast<unsigned>(current_position);
    if (remaining == 0)
    {
        current_bytes = 0;
    }
    else
    {
        current_bytes <<= current_position;
    }
    current_position = 0;
    valid_bits = remaining;

    const unsigned bytesToRead = (64 - valid_bits) / 8;

    if (bytesToRead == 0)
    {
        return false;
    }
    char bytes[8];
    inputFile.read(bytes, bytesToRead);
    const streamsize bytesRead = inputFile.gcount();

    if (inputFile.bad() || (inputFile.fail() && !inputFile.eof()))
    {
        throw runtime_error("Failed to read from input file");
    }
    for (streamsize i = 0; i < bytesRead; ++i)
    {
        const uint64_t byte =
            static_cast<unsigned char>(bytes[i]);

        current_bytes |= byte << (56 - valid_bits);
        valid_bits += 8;
    }

    return bytesRead > 0;
}

uint64_t BitReader::readBits(uint64_t size)
{
    assert(size <= 64);

    uint64_t result = 0;
    while (size != 0) {
        const unsigned chunk = static_cast<unsigned>(size > 56 ? 56 : size);
        uint64_t value;
        if (!tryPeekBits(chunk, value)) {
            throw runtime_error("Unexpected end of file while reading bits");
        }
        result = (result << chunk) | value;
        consumeBits(chunk);
        size -= chunk;
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


bool BitReader::tryPeekBits(unsigned size, uint64_t& value) {
    if (size > 56) {
        throw invalid_argument("BitReader peek size exceeds 56 bits");
    }
    if (size == 0) {
        value = 0;
        return true;
    }

    if (valid_bits - current_position < size) {
        refill();
        if (valid_bits - current_position < size) {
            return false;
        }
    }
    value = (current_bytes << current_position) >> (64 - size);
    return true;
}

void BitReader::consumeBits(unsigned size) {
    assert(size <= valid_bits - current_position);
    current_position += size;
}

uint8_t BitReader::readByte()
{
    return static_cast<uint8_t>(readBits(8));
}

BitReader::~BitReader()
{
}