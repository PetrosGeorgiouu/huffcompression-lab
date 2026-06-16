#include <iostream>
#include <unordered_map>
#include <chrono>
#include "findFrequenciesNaive.cpp"
#include "HuffmanTreeNaive.cpp"

using namespace std::chrono;

using namespace std;

int main()
{
     auto completestart = high_resolution_clock::now();
     auto start = high_resolution_clock::now();
     // std::string result = findStringNaive("complete_project_gutenberg_works_of_george_meredith.txt");
     std::string result = findStringNaive("random.txt");
     auto stop = high_resolution_clock::now();
     auto duration = duration_cast<nanoseconds>(stop - start);

     cout << "Time to get string (in nanoseconds): " << duration.count() << endl;

     cout << "\nOriginal string:\n"
          << result << "\n";

     start = high_resolution_clock::now();
     std::unordered_map<char, u_int64_t> freqs = findFrequenciesNaive(result);
     stop = high_resolution_clock::now();
     duration = duration_cast<nanoseconds>(stop - start);

     cout << "Time to get frequencies (in nanoseconds): " << duration.count() << endl;

     start = high_resolution_clock::now();

     HuffmanTreeNaive HuffmanTree = HuffmanTreeNaive(freqs);

     std::unordered_map<char, string>
         huffmanCode = HuffmanTree.getEncodings();
     stop = high_resolution_clock::now();
     duration = duration_cast<nanoseconds>(stop - start);

     cout << "Time to get encoding (in nanoseconds): " << duration.count() << endl;

     auto completestop = high_resolution_clock::now();
     auto completeduration = duration_cast<nanoseconds>(completestop - completestart);

     cout << "Time to encode/decode (in nanoseconds): " << duration.count() << endl;

     cout << "Total time (in nanoseconds): " << completeduration.count() << endl;

     return 0;
}