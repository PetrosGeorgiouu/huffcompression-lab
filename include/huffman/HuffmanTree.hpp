#pragma once

#include <array>
#include <cstdint>
#include <memory>

class BitWriter;

class HuffmanTree
{
public:
  struct Encoding
  {
    uint64_t encoding = 0;
    uint64_t size = 0;
    Encoding() = default;
    Encoding(uint64_t encoding, uint64_t size) : encoding(encoding), size(size) {}
  };
  explicit HuffmanTree(std::array<uint64_t, 256> &freqs);

  ~HuffmanTree();

  HuffmanTree(const HuffmanTree &) = delete;
  HuffmanTree &operator=(const HuffmanTree &) = delete;

  std::array<Encoding, 256> getEncodings();

  void serialize(BitWriter &bitWriter);

private:
  struct Node;

  std::unique_ptr<Node> root;

  void getEncodingsHelper(
      const Node &node,
      uint64_t code,
      uint64_t size,
      std::array<Encoding, 256> &encodings);

  void serializeHelper(BitWriter &bitWriter, const Node &current);
};
