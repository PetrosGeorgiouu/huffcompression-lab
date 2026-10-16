#pragma once

#include <iostream>
#include <memory>
#include "huffman/BitReader.hpp"

class HuffmanSerialized
{
public:
  explicit HuffmanSerialized(BitReader &bitReader);
  HuffmanSerialized(const HuffmanSerialized &) = delete;
  HuffmanSerialized &operator=(const HuffmanSerialized &) = delete;

  bool next(uint8_t bit);
  char leafAndReset();

private:
  struct Node;

  std::unique_ptr<Node> root = nullptr;
  std::unique_ptr<Node> current = nullptr;
};