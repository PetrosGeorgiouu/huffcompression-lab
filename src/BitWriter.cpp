#include <iostream>
#include <fstream>
#include <ostream>
#include <cassert>
#include <cstdint>
#include <array>
#include <algorithm>
#include "huffman/BitWriter.hpp"

using namespace std;

void BitWriter::bufferFlush() {
    if (buffer_bytes == 0) {
        return;
    }
    outputFile.write(reinterpret_cast<const char*>(buffer), buffer_bytes);
    buffer_bytes = 0;
}

void BitWriter::bufferByte(uint8_t byte) {
    buffer[buffer_bytes++] = byte;
    if (buffer_bytes == sizeof(buffer))
    {
        bufferFlush();
    }
}

void BitWriter::bufferWord(uint64_t word)
{
    for (int shift = 56; shift >= 0; shift -= 8)
    {
        const uint8_t byte =
            static_cast<uint8_t>(word >> shift);

        bufferByte(byte);
    }
}


BitWriter::BitWriter(ostream &output)
    : reservoir{0}, reservoir_size{0}, outputFile{output},buffer{{}}, buffer_bytes{0}
{
}

void BitWriter::writeBits(uint64_t bits, uint64_t size)
{
    assert(size <= 64);
    assert(size == 64 || (bits >> size) == 0);

    if (size == 0)
    {
        return;
    }

    assert(reservoir_size < 64);

    const uint8_t total =
        static_cast<uint8_t>(reservoir_size + size);
    if (total < 64)
    {
        reservoir = (reservoir << size) | bits;
        reservoir_size = total;
        return;
    }
    const Reservoir combined =
        (static_cast<Reservoir>(reservoir) << size) |
        static_cast<Reservoir>(bits);

    const uint8_t remaining =
        static_cast<uint8_t>(total - 64);

    const uint64_t word =
        static_cast<uint64_t>(combined >> remaining);

    bufferWord(word);

    const uint64_t remainingMask =
        (uint64_t{1} << remaining) - 1;

    reservoir =
        static_cast<uint64_t>(combined) & remainingMask;

    reservoir_size = remaining;
}

void BitWriter::writeBit(uint8_t bit)
{
    assert(bit == 0 || bit == 1);
    writeBits(static_cast<uint64_t>(bit), 1);
}

void BitWriter::writeByte(uint8_t byte)
{
    writeBits(static_cast<uint64_t>(byte), 8);
}


void BitWriter::flush()
{
    while (reservoir_size >= 8)
    {
        const uint8_t shift =
            static_cast<uint8_t>(reservoir_size - 8);

        const uint8_t byte =
            static_cast<uint8_t>(reservoir >> shift);

        bufferByte(byte);

        reservoir_size =
            static_cast<uint8_t>(reservoir_size - 8);

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
        const uint8_t byte =
            static_cast<uint8_t>(
                reservoir << (8 - reservoir_size));

        bufferByte(byte);
    }
    reservoir = 0;
    reservoir_size = 0;

    bufferFlush();
}

BitWriter::~BitWriter()
{
}