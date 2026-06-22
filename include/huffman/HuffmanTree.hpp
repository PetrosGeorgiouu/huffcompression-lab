#pragma once

#include <iostream>
#include <queue>
#include <unordered_map>
#include <vector>
using namespace std;

class HuffmanTreeNaive
{
public:
  explicit HuffmanTreeNaive(std::unordered_map<char, uint64_t> &freqs);

  ~HuffmanTreeNaive();

  HuffmanTreeNaive(const HuffmanTreeNaive &) = delete;
  HuffmanTreeNaive &operator=(const HuffmanTreeNaive &) = delete;

  std::unordered_map<char, std::string> getEncodings();

private:
  struct Node;

  Node *root = nullptr;

  void getEncodingsHelper(
      const Node *node,
      std::string code,
      std::unordered_map<char, std::string> &encodings);

  void deleteHelper(Node *node);

  struct compare;
};