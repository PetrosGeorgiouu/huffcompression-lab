#include <stdexcept>
#include <memory>
#include <utility>
#include "huffman/HuffmanDecoder.hpp"
#include "huffman/BitReader.hpp"

struct HuffmanDecoder::Node
{
  std::unique_ptr<Node> left;
  std::unique_ptr<Node> right;
  char symbol;

  Node(char ch,
       std::unique_ptr<Node> left = nullptr,
       std::unique_ptr<Node> right = nullptr)
      : left(std::move(left)),
        right(std::move(right)),
        symbol(ch)
  {
  }

  bool isLeaf() const noexcept
  {
    return !left && !right;
  }
};

HuffmanDecoder::HuffmanDecoder(BitReader &reader) : reader_(reader)
{
  root = deserializeHelper();
  buildLookupTable();
}

HuffmanDecoder::~HuffmanDecoder() = default;

std::unique_ptr<HuffmanDecoder::Node>
HuffmanDecoder::deserializeHelper()
{
  if (nodeCount_ >= MAX_NODES)
  {
    throw std::runtime_error("Invalid Huffman tree: exceeds 511 nodes.");
  }
  ++nodeCount_;
  const std::uint8_t marker = reader_.readBit();
  if (marker == 1)
  {
    const char ch = static_cast<char>(reader_.readByte());
    return std::make_unique<Node>(ch);
  }
  auto left = deserializeHelper();
  auto right = deserializeHelper();
  return std::make_unique<Node>('\0', std::move(left), std::move(right));
}

void HuffmanDecoder::buildLookupTable() {
  table_.fill(Entry{});
  if (!root || root -> isLeaf()) {
    return;
  }
  for (std::size_t pattern = 0; pattern < TABLE_SIZE; ++pattern) {
    const Node* current = root.get();
    std::uint8_t consumed = 0;

    while (consumed < LOOKUP_BITS && !current -> isLeaf()) {
      const std::size_t shift = LOOKUP_BITS - 1 - consumed;
      const bool bit = ((pattern >> shift) & std::size_t{1}) != 0;
      current = bit? current -> right.get(): current -> left.get();
      ++consumed;

      if (!current) {
        throw std::runtime_error("Invalid Huffman tree.");
      }

    }
    if (current -> isLeaf()) {
      table_[pattern] = Entry{current -> symbol, consumed};
    }
  }
}

void HuffmanDecoder::decodeBlock(
    char* output,
    std::size_t count) const
{
  if (count == 0)
  {
    return;
  }

  const Node* rootNode = root.get();

  if (!rootNode)
  {
    throw std::runtime_error("Can't decode an empty tree");
  }

  // Select single-symbol handling once for the entire block.
  if (rootNode->isLeaf())
  {
    const char symbol = rootNode->symbol;

    for (std::size_t i = 0; i < count; ++i)
    {
      if (reader_.readBit() != 0)
      {
        throw std::runtime_error(
            "Invalid bit for single symbol tree.");
      }

      output[i] = symbol;
    }

    return;
  }

  for (std::size_t i = 0; i < count; ++i)
  {
    std::uint64_t pattern;

    // Fast path: decode using the lookup table.
    if (reader_.tryPeekBits(LOOKUP_BITS, pattern))
    {
      const Entry& entry = table_[pattern];

      if (entry.bitsConsumed != 0)
      {
        reader_.consumeBits(entry.bitsConsumed);
        output[i] = entry.symbol;
        continue;
      }
    }

    // Fallback: long code or insufficient lookahead.
    const Node* current = rootNode;

    while (!current->isLeaf())
    {
      const std::uint8_t bit = reader_.readBit();

      current = bit == 0
          ? current->left.get()
          : current->right.get();

      if (!current)
      {
        throw std::runtime_error("Invalid Huffman path.");
      }
    }

    output[i] = current->symbol;
  }
}
