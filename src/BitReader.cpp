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

bool BitReader::refillInputBuffer() {
    if (input_eof_) {
        return false;
    }
    inputFile.read(input_buffer_.data(), static_cast<streamsize>(input_buffer_.size()));
    const streamsize bytesRead = inputFile.gcount();
    if (inputFile.bad() || (inputFile.fail() && !inputFile.eof())){
        throw runtime_error("Failed to read from input file");
    }
    input_position_ = 0;
    input_size_ = static_cast<std::size_t>(bytesRead);
    input_eof_ = inputFile.eof();
    return input_size_ != 0;
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
    unsigned appended = 0;
    for (unsigned i = 0; i < bytesToRead; ++i)
    {
        if (input_position_ == input_size_ && !refillInputBuffer())
        {
            break;
        }

        const uint64_t byte =
            static_cast<unsigned char>(input_buffer_[input_position_++]);
        current_bytes |= byte << (56 - valid_bits);
        valid_bits += 8;
        ++appended;
    }
    return appended != 0;
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

uint8_t BitReader::readByte()
{
    return static_cast<uint8_t>(readBits(8));
}

BitReader::~BitReader()
{
}