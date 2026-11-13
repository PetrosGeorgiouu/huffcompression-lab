#include "huffman/BitWriter.hpp"

#include <cassert>
#include <cstdint>
#include <ostream>

void BitWriter::bufferFlush()
{
    if (buffer_bytes == 0)
    {
        return;
    }

    outputFile.write(
        reinterpret_cast<const char *>(buffer),
        static_cast<std::streamsize>(buffer_bytes));

    buffer_bytes = 0;
}

void BitWriter::bufferByte(std::uint8_t byte)
{
    buffer[buffer_bytes++] = byte;

    if (buffer_bytes == sizeof(buffer))
    {
        bufferFlush();
    }
}

void BitWriter::bufferWord(std::uint64_t word)
{
    for (int shift = 56; shift >= 0; shift -= 8)
    {
        const std::uint8_t byte =
            static_cast<std::uint8_t>(word >> shift);

        bufferByte(byte);
    }
}

BitWriter::BitWriter(std::ostream &output)
    : reservoir{0},
      reservoir_size{0},
      outputFile{output},
      buffer{},
      buffer_bytes{0}
{
}

void BitWriter::drainBits(
    std::uint64_t bits,
    std::uint64_t size,
    std::uint8_t total)
{
    assert(size <= 64);
    assert(total >= 64 && total < 128);

    const Reservoir combined =
        (static_cast<Reservoir>(reservoir) << size) |
        static_cast<Reservoir>(bits);

    const std::uint8_t remaining =
        static_cast<std::uint8_t>(total - 64);

    const std::uint64_t word =
        static_cast<std::uint64_t>(combined >> remaining);

    bufferWord(word);

    const std::uint64_t remainingMask =
        (std::uint64_t{1} << remaining) - 1;

    reservoir =
        static_cast<std::uint64_t>(combined) & remainingMask;

    reservoir_size = remaining;
}

void BitWriter::writeBits(
    std::uint64_t bits,
    std::uint64_t size)
{
    assert(size <= 64);
    assert(size == 64 || (bits >> size) == 0);

    if (size == 0)
    {
        return;
    }

    assert(reservoir_size < 64);

    const std::uint8_t total =
        static_cast<std::uint8_t>(reservoir_size + size);

    if (total < 64)
    {
        reservoir = (reservoir << size) | bits;
        reservoir_size = total;
        return;
    }

    drainBits(bits, size, total);
}

void BitWriter::writeBit(std::uint8_t bit)
{
    assert(bit == 0 || bit == 1);
    writeBits(static_cast<std::uint64_t>(bit), 1);
}

void BitWriter::writeByte(std::uint8_t byte)
{
    writeBits(static_cast<std::uint64_t>(byte), 8);
}

void BitWriter::flush()
{
    while (reservoir_size >= 8)
    {
        const std::uint8_t shift =
            static_cast<std::uint8_t>(reservoir_size - 8);

        const std::uint8_t byte =
            static_cast<std::uint8_t>(reservoir >> shift);

        bufferByte(byte);

        reservoir_size =
            static_cast<std::uint8_t>(reservoir_size - 8);

        if (reservoir_size == 0)
        {
            reservoir = 0;
        }
        else
        {
            const Reservoir remainingMask =
                (Reservoir{1} << reservoir_size) - 1;

            reservoir &= remainingMask;
        }
    }

    if (reservoir_size > 0)
    {
        const std::uint8_t byte =
            static_cast<std::uint8_t>(
                reservoir << (8 - reservoir_size));

        bufferByte(byte);
    }

    reservoir = 0;
    reservoir_size = 0;

    bufferFlush();
}