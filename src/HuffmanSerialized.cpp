#include <iostream>
#include <memory>
#include "huffman/BitReader.hpp"
#include "huffman/HuffmanSerialized.hpp"

struct HuffmanSerialized::Node
{
  char ch;
  std::unique_ptr<Node> left;
  std::unique_ptr<Node> right;
  Node(char ch)
      : ch(ch), left(nullptr), right(nullptr)
  {
  }
  Node(char ch, std::unique_ptr<Node> left_, std::unique_ptr<Node> right_)
      : ch(ch), left(std::move(left_)), right(std::move(right_))
  {
  }
  bool isLeaf() const
  {
    return left == nullptr && right == nullptr;
  }
};

HuffmanSerialized::HuffmanSerialized(BitReader &bitReader)
{
}
