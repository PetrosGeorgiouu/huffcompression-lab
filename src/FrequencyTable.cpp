#include <iostream>
#include <fstream>
#include <array>
#include "huffman/FrequencyTable.hpp"

using namespace std;

array <uint64_t, 256> findFrequencies(const string &txtPath)
{
  ifstream file(txtPath, ios::binary);
  array <uint64_t, 256> freqs{};

  char buffer[4096];
  while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
    streamsize count = file.gcount();
    for (streamsize i = 0; i < count; i++) {
      const auto c = static_cast<unsigned char>(buffer[i]);
      ++freqs[c];
    }
  }
  return freqs;
}