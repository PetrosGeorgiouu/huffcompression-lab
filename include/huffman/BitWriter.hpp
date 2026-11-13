#pragma once

#include <cstddef>
#include <cstdint>
#include <ostream>

class BitWriter
{
public:
    explicit BitWriter(std::ostream &output);

    void writeBits(std::uint64_t bits, std::uint64_t size);
    void writeBit(std::uint8_t bit);
    void writeByte(std::uint8_t byte);
    void flush();

private:
    using Reservoir = unsigned __int128;

    std::uint64_t reservoir;
    std::uint8_t reservoir_size;

    std::ostream &outputFile;

    static constexpr std::size_t BUFFER_SIZE = 64 * 1024;
    std::uint8_t buffer[BUFFER_SIZE];
    std::size_t buffer_bytes;

    void bufferFlush();
    void bufferByte(std::uint8_t byte);
    void bufferWord(std::uint64_t word);

    [[gnu::noinline]]
    void drainBits(
        std::uint64_t bits,
        std::uint64_t size,
        std::uint8_t total);
};