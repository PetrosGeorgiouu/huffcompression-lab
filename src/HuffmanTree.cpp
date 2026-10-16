#include <algorithm>
#include <memory>
#include <utility>

#include "huffman/BitWriter.hpp"
#include "huffman/HuffmanTree.hpp"
#include "huffman/UniquePtrPriorityQueue.hpp"

struct HuffmanTree::Node
{
  char ch;
  uint64_t freq;
  std::unique_ptr<Node> left;
  std::unique_ptr<Node> right;

  Node(char ch, uint64_t freq,
       std::unique_ptr<Node> left = nullptr,
       std::unique_ptr<Node> right = nullptr)
      : ch(ch), freq(freq), left(std::move(left)), right(std::move(right))
  {
  }

  bool isLeaf() const noexcept
  {
    return !left && !right;
  }
};

HuffmanTree::HuffmanTree(std::array<uint64_t, 256> &freqs)
{
  const auto compare = [](const Node &left, const Node &right) noexcept
  {
    return left.freq > right.freq;
  };

  const auto numChars = static_cast<std::size_t>(
      std::count_if(freqs.begin(), freqs.end(),
                    [](uint64_t frequency)
                    { return frequency > 0; }));

  UniquePtrPriorityQueue<Node, decltype(compare)> pq(numChars, compare);

  for (std::size_t i = 0; i < freqs.size(); ++i)
  {
    if (freqs[i] > 0)
    {
      pq.push(std::make_unique<Node>(static_cast<char>(i), freqs[i]));
    }
  }

  if (pq.empty())
  {
    return;
  }

  while (pq.size() > 1)
  {
    auto left = pq.extract_top();
    auto right = pq.extract_top();
    const uint64_t sum = left->freq + right->freq;
    pq.push(std::make_unique<Node>(
        '\0', sum, std::move(left), std::move(right)));
  }

  root = pq.extract_top();
}

void HuffmanTree::getEncodingsHelper(
    const Node &node, uint64_t code, uint64_t size,
    std::array<Encoding, 256> &encodings)
{
  if (node.isLeaf())
  {
    encodings[static_cast<unsigned char>(node.ch)] = Encoding(code, size);
    return;
  }

  if (node.left)
  {
    getEncodingsHelper(*node.left, code << 1, size + 1, encodings);
  }
  if (node.right)
  {
    getEncodingsHelper(*node.right, (code << 1) | 1ULL, size + 1, encodings);
  }
}

void HuffmanTree::serializeHelper(BitWriter &bitWriter, const Node &current)
{
  if (current.isLeaf())
  {
    bitWriter.writeBit(1);
    bitWriter.writeByte(static_cast<std::uint8_t>(static_cast<unsigned char>(current.ch)));
    return;
  }

  bitWriter.writeBit(0);
  if (current.left)
  {
    serializeHelper(bitWriter, *current.left);
  }
  if (current.right)
  {
    serializeHelper(bitWriter, *current.right);
  }
}

std::array<HuffmanTree::Encoding, 256> HuffmanTree::getEncodings()
{
  std::array<Encoding, 256> encodingTable{};
  if (!root)
  {
    return encodingTable;
  }
  if (root->isLeaf())
  {
    encodingTable[static_cast<unsigned char>(root->ch)] = Encoding(0, 1);
  }
  else
  {
    getEncodingsHelper(*root, 0, 0, encodingTable);
  }
  return encodingTable;
}

void HuffmanTree::serialize(BitWriter &bitWriter)
{
  if (root)
  {
    serializeHelper(bitWriter, *root);
  }
}

HuffmanTree::~HuffmanTree() = default;
