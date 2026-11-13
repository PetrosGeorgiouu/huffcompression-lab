#include "huffman/BitReader.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <istream>
#include <stdexcept>

namespace
{
    std::uint64_t loadBigEndianWord(const char *bytes)
    {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        std::uint64_t word;
        std::memcpy(&word, bytes, sizeof(word));
        return __builtin_bswap64(word);
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        std::uint64_t word;
        std::memcpy(&word, bytes, sizeof(word));
        return word;
#else
        std::uint64_t word = 0;

        for (unsigned i = 0; i < 8; ++i)
        {
            word = (word << 8) |
                   static_cast<unsigned char>(bytes[i]);
        }

        return word;
#endif
    }
}

BitReader::BitReader(std::istream &input)
    : inputFile{input},
      current_bytes{0},
      current_position{0},
      valid_bits{0}
{
}

bool BitReader::refillInputBuffer()
{
    if (input_eof_)
    {
        return false;
    }

    inputFile.read(
        input_buffer_.data(),
        static_cast<std::streamsize>(input_buffer_.size()));

    const std::streamsize bytesRead = inputFile.gcount();

    if (inputFile.bad() || (inputFile.fail() && !inputFile.eof()))
    {
        throw std::runtime_error("Failed to read from input file");
    }

    input_position_ = 0;
    input_size_ = static_cast<std::size_t>(bytesRead);
    input_eof_ = inputFile.eof();

    return input_size_ != 0;
}

bool BitReader::refill()
{
    // Preserve unread bits at the most significant end.
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

    if (input_position_ == input_size_ && !refillInputBuffer())
    {
        return false;
    }

    // Load eight valid bytes, consuming only those that fit.
    if (input_size_ - input_position_ >= sizeof(std::uint64_t))
    {
        std::uint64_t word =
            loadBigEndianWord(input_buffer_.data() + input_position_);

        const unsigned addedBits = bytesToRead * 8;

        word >>= 64 - addedBits;
        current_bytes |= word << (64 - valid_bits - addedBits);

        input_position_ += bytesToRead;
        valid_bits += addedBits;

        return true;
    }

    // Handle input-buffer boundaries and partial final blocks.
    unsigned appended = 0;

    for (unsigned i = 0; i < bytesToRead; ++i)
    {
        if (input_position_ == input_size_ && !refillInputBuffer())
        {
            break;
        }

        const std::uint64_t byte =
            static_cast<unsigned char>(
                input_buffer_[input_position_++]);

        current_bytes |= byte << (56 - valid_bits);
        valid_bits += 8;
        ++appended;
    }

    return appended != 0;
}

std::uint64_t BitReader::readBits(std::uint64_t size)
{
    assert(size <= 64);

    std::uint64_t result = 0;

    while (size != 0)
    {
        const unsigned chunk =
            static_cast<unsigned>(size > 56 ? 56 : size);

        std::uint64_t value;

        if (!tryPeekBits(chunk, value))
        {
            throw std::runtime_error(
                "Unexpected end of file while reading bits");
        }

        result = (result << chunk) | value;
        consumeBits(chunk);
        size -= chunk;
    }

    return result;
}

std::uint8_t BitReader::readBit()
{
    if (current_position == valid_bits && !refill())
    {
        throw std::runtime_error(
            "Unexpected end of file while reading bits");
    }

    const std::uint8_t bit =
        static_cast<std::uint8_t>(
            current_bytes >> (63 - current_position)) & 1;

    ++current_position;
    return bit;
}

std::uint8_t BitReader::readByte()
{
    return static_cast<std::uint8_t>(readBits(8));
}