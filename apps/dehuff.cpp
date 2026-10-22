#include "huffman/Decompression.hpp"

#include <exception>
#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    std::cerr << "Usage: " << argv[0]
              << " <input-file> <output-file>\n";
    return 1;
  }

  const std::string inputPath = argv[1];
  const std::string outputPath = argv[2];

  try
  {
    decompressor(inputPath, outputPath);
  }
  catch (const std::exception &error)
  {
    std::cerr << "Decompression failed: " << error.what() << '\n';
    return 1;
  }

  std::cout << "Decompressed: " << inputPath << " -> " << outputPath << '\n';
  return 0;
}
