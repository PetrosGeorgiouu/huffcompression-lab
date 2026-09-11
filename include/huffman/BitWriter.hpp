#pragma once
#include <cstdint>
#include <ostream>
#include <array>

using namespace std;

class BitWriter
{

public:
    explicit BitWriter(ostream &output);
    ~BitWriter();
    void writeBits(uint64_t bits, uint64_t size);
    void writeBit(uint8_t bit);
    void writeByte(uint8_t byte);
    void flush();

private:

    using Reservoir = unsigned __int128;
    Reservoir reservoir;
    uint8_t reservoir_size;

    ostream &outputFile;
    static constexpr std::size_t BUFFER_SIZE = 64 * 1024;
    uint8_t buffer[BUFFER_SIZE];
    size_t buffer_bytes;

    void bufferFlush();
    void bufferByte(uint8_t byte);
    void bufferWord(uint64_t word);
};