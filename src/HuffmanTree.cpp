// C++ program to implement huffman coding

#include <iostream>
#include <queue>
#include <unordered_map>
#include <vector>
#include "huffman/HuffmanTree.hpp"
using namespace std;

struct HuffmanTreeNaive::Node
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
};

HuffmanTreeNaive::HuffmanTreeNaive(unordered_map<char, uint64_t> &freqs)
{
  const auto compare = [](const Node *left, Node *right)
  {
    return left->freq < right->freq;
  };
  priority_queue<Node *, vector<Node *>, decltype(compare)>
      pq(compare);
  for (auto pair : freqs)
  {
    if (pair.second > 0)
    {
      pq.push(new Node(pair.first, pair.second));
    }
  }
  while (pq.size() != 1)
  {
    Node *left = pq.top();
    pq.pop();
    Node *right = pq.top();
    pq.pop();
    int sum = left->freq + right->freq;
    pq.push(new Node('\0', sum, left, right));
  }
  this->root = pq.top();
}

void HuffmanTreeNaive::getEncodingsHelper(const Node *node, string str,
                                          unordered_map<char, string> &encodings)
{
  if (node == nullptr)
    return;

  if (!node->left && !node->right)
  {
    encodings[node->ch] = str;
  }

  getEncodingsHelper(node->left, str + "0", encodings);
  getEncodingsHelper(node->right, str + "1", encodings);
}

void HuffmanTreeNaive::deleteHelper(Node *node)
{
  if (node != nullptr)
  {
    deleteHelper(node->left);
    deleteHelper(node->right);
    delete node;
  }
}

unordered_map<char, string> HuffmanTreeNaive::getEncodings()
{
  unordered_map<char, string> encodingTable;
  getEncodingsHelper(this->root, "", encodingTable);
  return encodingTable;
}

HuffmanTreeNaive::~HuffmanTreeNaive()
{
  deleteHelper(this->root);
}