#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

class BitReader;
class HuffmanDecoder
{
public:
  explicit HuffmanDecoder(BitReader &reader);
  char decodeSymbol() const;
  ~HuffmanDecoder();

  HuffmanDecoder(const HuffmanDecoder &) = delete;
  HuffmanDecoder &operator=(const HuffmanDecoder &) = delete;

private:
  static constexpr uint16_t noChild =
      std::numeric_limits<uint16_t>::max();
  struct Node
  {
    uint16_t left = noChild;
    uint16_t right = noChild;
    char symbol = {};

    bool isLeaf() const noexcept
    {
      return left == noChild;
    }
  };
  BitReader &reader_;
  std::size_t size = 0;
  std::array<Node, 511> tree{};
  uint16_t deserializeHelper();
};
