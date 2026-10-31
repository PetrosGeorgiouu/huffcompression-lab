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
  char decodeSymbol() const;
  ~HuffmanDecoder();

  HuffmanDecoder(const HuffmanDecoder &) = delete;
  HuffmanDecoder &operator=(const HuffmanDecoder &) = delete;

private:
  struct Node;
  std::unique_ptr<Node> root;
  // std::size_t size = 0;
  BitReader &reader_;
  std::unique_ptr<Node> deserializeHelper();
};
