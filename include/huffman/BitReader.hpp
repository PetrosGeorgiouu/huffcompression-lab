#pragma once
#include <cstdint>
#include <fstream>
#include <cassert>

using namespace std;

class BitReader
{
    public:
        explicit BitReader(istream &input);
        ~BitReader();
        uint64_t readBits(uint64_t size);
        uint8_t readBit();
        uint8_t readByte();

        bool tryPeekBits(unsigned size, uint64_t& value) {
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
        
        void consumeBits(unsigned size) {
            assert(size <= valid_bits - current_position);
            current_position += size;
        }

    private:
        istream &inputFile;
        uint64_t current_bytes;
        uint64_t current_position;
        unsigned valid_bits;

        bool refill();
};