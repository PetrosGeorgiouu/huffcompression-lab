#pragma once
#include <cstdint>
#include <fstream>

using namespace std;

class BitReader
{
    public:
        explicit BitReader(istream &input);
        ~BitReader();
        uint64_t readBits(uint64_t size);
        uint8_t readBit();
        uint8_t readByte();

    private:
        istream &inputFile;
        uint64_t current_bytes;
        uint64_t current_position;
        unsigned valid_bits;

        bool refill();
};