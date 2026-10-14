#pragma once

#include <iostream>
#include <queue>
#include <array>
#include <vector>
#include <unordered_map>
#include "huffman/BitWriter.hpp"
#include "huffman/FrequencyTable.hpp"
#include "huffman/BitReader.hpp"

using namespace std;

class HuffmanTree
{
public:

  struct Encoding {
    uint64_t encoding = 0;
    uint64_t size = 0;
    Encoding() = default;
    Encoding(uint64_t encoding, uint64_t size) : encoding(encoding), size(size) {}
    };
  explicit HuffmanTree(array <uint64_t, 256> &freqs);
  // explicit HuffmanTree(BitReader& reader);

  ~HuffmanTree();

  HuffmanTree(const HuffmanTree &) = delete;
  HuffmanTree &operator=(const HuffmanTree &) = delete;

  array <Encoding, 256> getEncodings();

  void serialize(BitWriter &bitWriter);

private:
  struct Node;

  Node *root = nullptr;

  void getEncodingsHelper(
      const Node *node, 
      uint64_t code, 
      uint64_t size,
      array<Encoding, 256> &encodings);

  void deleteHelper(Node *node);

  struct compare;

  void serializeHelper(BitWriter &bitWriter, Node *current);
};