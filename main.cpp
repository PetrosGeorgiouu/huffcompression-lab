#include <iostream>
#include <unordered_map>
#include <chrono>
#include "readFileNaive.cpp"
#include "findFrequenciesNaive.cpp"
#include "HuffmanCodeNaive.cpp"

using namespace std::chrono;

int main()
{
  auto completestart = high_resolution_clock::now();
  string pdfPath = "Systems.Performance.Enterprise.and.the.Cloud.2nd.Edition.2020.12.pdf";

  auto start = high_resolution_clock::now();
  extractFileNaive(pdfPath);
  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<nanoseconds>(stop - start);

  cout << "Time to extract pdf (in nanoseconds): " << duration.count() << endl;

  start = high_resolution_clock::now();
  std::string result = findStringNaive("output.txt");
  stop = high_resolution_clock::now();
  duration = duration_cast<nanoseconds>(stop - start);

  cout << "Time to get string (in nanoseconds): " << duration.count() << endl;

  start = high_resolution_clock::now();
  std::unordered_map<char, int> freqs = findFrequenciesNaive(result);
  stop = high_resolution_clock::now();
  duration = duration_cast<nanoseconds>(stop - start);

  cout << "Time to get frequencies (in nanoseconds): " << duration.count() << endl;

  start = high_resolution_clock::now();
  buildHuffmanTreeNaive(result, freqs);
  stop = high_resolution_clock::now();
  duration = duration_cast<nanoseconds>(stop - start);

  auto completestop = high_resolution_clock::now();
  auto completeduration = duration_cast<nanoseconds>(completestop - completestart);

  cout << "Time to encode/decode (in nanoseconds): " << duration.count() << endl;

  cout << "Total time (in nanoseconds): " << completeduration.count() << endl;

  return 0;
}