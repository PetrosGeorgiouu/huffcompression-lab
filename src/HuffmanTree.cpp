#include <iostream>
#include <queue>
#include <unordered_map>
#include <vector>
#include <array>
#include <cstdint>
#include <algorithm>
#include "huffman/BitWriter.hpp"
#include "huffman/HuffmanTree.hpp"
using namespace std;

struct HuffmanTree::Node
{
  char ch;
  uint64_t freq;
  Node *left;
  Node *right;
  Node(char ch, uint64_t freq)
      : ch(ch), freq(freq), left(nullptr), right(nullptr)
  {
  }
  Node(char ch, uint64_t freq, Node *left, Node *right)
      : ch(ch), freq(freq), left(left), right(right)
  {
  }
  bool isLeaf() const
  {
    return left == nullptr && right == nullptr;
  }
};

HuffmanTree::HuffmanTree(array <uint64_t, 256> &freqs)
{
  const auto compare = [](const Node *left, Node *right)
  {
    return left->freq > right->freq;
  };

  priority_queue<Node *, vector<Node *>, decltype(compare)>
      pq(compare);
  int numChars = 0;
  for (int i = 0; i < 256; i++)
  {
    if (freqs[i] > 0)
    {
      pq.push(new Node(static_cast<char>(i), freqs[i]));
      numChars++;
    }
  }
  if (!numChars) {
    this->root = nullptr;
    return;
  }
  if (numChars == 1) {
    this->root = pq.top();
    return;
  }
  while (pq.size() != 1)
  {
    Node *left = pq.top();
    pq.pop();
    Node *right = pq.top();
    pq.pop();
    uint64_t sum = left->freq + right->freq;
    pq.push(new Node{'\0', sum, left, right});
  }
  this->root = pq.top();
}

// HuffmanTree::HuffmanTree(BitReader& reader)
//   : root{nullptr}
// {
  
// }

void HuffmanTree::getEncodingsHelper(const Node *node, uint64_t code, uint64_t size,
    array<Encoding, 256> &encodings)
{
  if (node == nullptr)
    return;

  if (!node->left && !node->right)
  {
    encodings[static_cast<unsigned char>(node->ch)] = Encoding(code, size);
  }

  getEncodingsHelper(node->left, static_cast<uint64_t>((code << 1) | 0ULL), size + 1, encodings);
  getEncodingsHelper(node->right, static_cast<uint64_t>((code << 1) | 1ULL), size + 1, encodings);
}

void HuffmanTree::deleteHelper(Node *node)
{
  if (node != nullptr)
  {
    deleteHelper(node->left);
    deleteHelper(node->right);
    delete node;
  }
}

void HuffmanTree::serializeHelper(BitWriter &bitWriter, Node *current)
{
  if (current != nullptr)
  {
    if (current->isLeaf())
    {
      bitWriter.writeBit(1);
      bitWriter.writeByte(static_cast<std::uint8_t>(static_cast<unsigned char>(current->ch)));
    }
    else
    {
      bitWriter.writeBit(0);
    }
    serializeHelper(bitWriter, current->left);
    serializeHelper(bitWriter, current->right);
  }
}

array<HuffmanTree::Encoding, 256> HuffmanTree::getEncodings()
{
  array<Encoding, 256> encodingTable = {};
  if (this->root == nullptr)
  {
    return encodingTable;
  }
  if (this->root->left == nullptr && this->root->right == nullptr)
  {
    encodingTable[static_cast<unsigned char>(this->root->ch)] = HuffmanTree::Encoding(0, 1);
  }
  else
  {
    getEncodingsHelper(this->root, 0, 0, encodingTable);
  }
  return encodingTable;
}

void HuffmanTree::serialize(BitWriter &bitWriter)
{
  serializeHelper(bitWriter, this->root);
}

HuffmanTree::~HuffmanTree()
{
  deleteHelper(this->root);
}