#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>

class BitReader;
class HuffmanDecoder
{
public:
  explicit HuffmanDecoder(BitReader &reader);

  void decodeBlock(char* output, std::size_t count) const;
  ~HuffmanDecoder();

  HuffmanDecoder(const HuffmanDecoder &) = delete;
  HuffmanDecoder &operator=(const HuffmanDecoder &) = delete;

private:
  struct Node;
  struct Entry
  {
    char symbol = '\0';
    uint8_t bitsConsumed = 0;
  };

static_assert(sizeof(Entry) == 2,
              "Expected a two-byte lookup entry");

  static constexpr std::size_t LOOKUP_BITS = 10;
  static constexpr std::size_t TABLE_SIZE = std::size_t{1} << LOOKUP_BITS; 
  std::array<Entry, TABLE_SIZE> table_{};
  void buildLookupTable();
  std::unique_ptr<Node> root;
  static constexpr std::size_t MAX_NODES = 2 * 256 - 1;
  std::size_t nodeCount_ = 0;
  BitReader &reader_;
  std::unique_ptr<Node> deserializeHelper();
};
