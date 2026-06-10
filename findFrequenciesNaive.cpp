#include <iostream>
#include <fstream>
#include <unordered_map>

std::unordered_map<char, int> findFrequenciesNaive(std::string txt)
{
  std::unordered_map<char, int> freqs;

  for (char c : txt)
  {
    if (freqs.find(c) != freqs.begin())
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

std::string findStringNaive(std::string txtPath)
{
  std::ifstream file(txtPath);
  std::string str;
  std::string result;
  while (std::getline(file, str))
  {
    result += str;
  }
  return result;
}