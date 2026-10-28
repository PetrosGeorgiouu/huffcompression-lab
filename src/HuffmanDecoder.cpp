#include <stdexcept>
#include "huffman/HuffmanDecoder.hpp"
#include "huffman/BitReader.hpp"

HuffmanDecoder::HuffmanDecoder(BitReader &reader) : reader_(reader)
{
  deserializeHelper();
}

HuffmanDecoder::~HuffmanDecoder() = default;

uint16_t HuffmanDecoder::deserializeHelper()
{
  if (size >= tree.size())
  {
    throw std::runtime_error("Huffman Tree exceeds bounded capacity.");
  }
  const uint16_t index = static_cast<uint16_t>(size++);
  const uint8_t marker = reader_.readBit();
  if (marker == 1)
  {
    tree[index].symbol = static_cast<char>(reader_.readByte());
  }
  else
  {
    const uint16_t left = deserializeHelper();
    const uint16_t right = deserializeHelper();

    tree[index].left = left;
    tree[index].right = right;
  }
  return index;
}

char HuffmanDecoder::decodeSymbol() const
{
  if (size == 0)
  {
    throw std::runtime_error("Can't decode an empty tree");
  }
  uint16_t current = 0;
  if (tree[current].isLeaf())
  {
    if (reader_.readBit() != 0)
    {
      throw std::runtime_error("Invalid bit for single symbol tree.");
    }
    return tree[current].symbol;
  }
  while (!tree[current].isLeaf())
  {
    const uint8_t next_bit = reader_.readBit();
    current = next_bit == 0 ? tree[current].left : tree[current].right;
    if (current >= size)
    {
      throw std::runtime_error("Invalid Huffman path.");
    }
  }
  return tree[current].symbol;
}