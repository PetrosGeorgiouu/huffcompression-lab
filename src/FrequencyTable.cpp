#include <iostream>
#include <fstream>
#include <unordered_map>
#include "huffman/FrequencyTable.hpp"

using namespace std;

unordered_map<char, uint64_t> findFrequenciesNaive(const string &txtPath)
{
  ifstream file(txtPath);
  string str;
  string result;
  while (getline(file, str))
  {
    result += str;
  }
  unordered_map<char, u_int64_t> freqs;

  for (char c : result)
  {
    if (freqs.find(c) != freqs.end())
    {
      freqs[c]++;
    }
    else
    {
      freqs[c] = 1;
    }
  }
  return freqs;
}