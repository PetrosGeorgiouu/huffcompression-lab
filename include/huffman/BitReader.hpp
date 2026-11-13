#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <stdexcept>

class BitReader
{
public:
    explicit BitReader(std::istream &input);

    std::uint64_t readBits(std::uint64_t size);
    std::uint8_t readBit();
    std::uint8_t readByte();

    bool tryPeekBits(unsigned size, std::uint64_t &value)
    {
        if (size > 56)
        {
            throw std::invalid_argument(
                "BitReader peek size exceeds 56 bits");
        }

        if (size == 0)
        {
            value = 0;
            return true;
        }

        if (valid_bits - current_position < size)
        {
            refill();

            if (valid_bits - current_position < size)
            {
                return false;
            }
        }

        value = (current_bytes << current_position) >> (64 - size);
        return true;
    }

    void consumeBits(unsigned size)
    {
        assert(size <= valid_bits - current_position);
        current_position += size;
    }

private:
    std::istream &inputFile;
    std::uint64_t current_bytes;
    std::uint64_t current_position;
    unsigned valid_bits;

    static constexpr std::size_t INPUT_BUFFER_SIZE = 64 * 1024;

    std::array<char, INPUT_BUFFER_SIZE> input_buffer_;
    std::size_t input_position_ = 0;
    std::size_t input_size_ = 0;
    bool input_eof_ = false;

    bool refillInputBuffer();
    bool refill();
};