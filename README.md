# Huffman Data Compression Lab
A data compression library for compressing/decompressing text files. Compares a naive implementation to a performance enhanced implementation.

# An Explanation of the Naive Pipeline

We first explain the pipeline developed via a naive solution. First, we take a .txt and convert it to a string object, and then count the frequencies of each character in the string via a unordered_map<char, int> object. We then use a Huffman coding algorithm that continuosly takes the two lowest frequency characters and makes them leaves in a Huffman Tree. A character gets an assigned binary value by starting from the root and performing a preorder traversal, and keeping track, in order of when we've traversed left, with the character 0, or right, with the character 1. One a leaf is found, assign the character the binary number constructed, and the traverse to the next leaf while removing the binary numbers constructed in that order. Each character now has a prefix free encoding that minimizes the size of the encoded file.

We then encode the characters, as well as decode them to show we preserve loselessness.

# How I Improved Performance

I'd also like to take some time to explain some of my optimizations and the actual statistics as to how my new model compares to the naive model.

## Acknowledgments

Early versions of my Huffman Coding file were inspired by a C++ implementation done by GeeksforGeeks (https://www.geeksforgeeks.org/cpp/huffman-coding-in-cpp/?_x_tr_hist=true); I also used their tutorial to read from files using C++ (https://www.w3schools.com/cpp/cpp_files.asp).
