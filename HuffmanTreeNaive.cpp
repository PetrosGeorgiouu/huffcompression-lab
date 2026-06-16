// C++ program to implement huffman coding

#include <iostream>
#include <queue>
#include <unordered_map>
#include <vector>
using namespace std;

class HuffmanTreeNaive
{
private:
  struct Node
  {
    char ch;
    int freq;
    Node *left;
    Node *right;
    Node(char ch, int freq)
        : ch(ch), freq(freq), left(nullptr), right(nullptr)
    {
    }
    Node(char ch, int freq, Node *left, Node *right)
        : ch(ch), freq(freq), left(left), right(right)
    {
    }
  };

  Node *root;

  struct compare
  {
    bool operator()(Node *l, Node *r) const
    {
      return l->freq > r->freq;
    }
  };

  void getEncodingsHelper(const Node *node, string str,
                          unordered_map<char, string> &encodings)
  {
    if (node == nullptr)
      return;

    if (!node->left && !node->right)
    {
      encodings[node->ch] = str;
    }

    getEncodingsHelper(node->left, str + "0", encodings);
    getEncodingsHelper(node->right, str + "1", encodings);
  }

  void deleteHelper(Node *node)
  {
    if (node != nullptr)
    {
      deleteHelper(node->left);
      deleteHelper(node->right);
      delete node;
    }
  }

public:
  struct Code
  {
    uint64_t code;
    uint64_t size;
  };

  explicit HuffmanTreeNaive(const unordered_map<char, uint64_t> &freq, bool encode = false)
  {
    priority_queue<Node *, vector<Node *>, compare> pq;
    for (auto pair : freq)
    {
      if (pair.second > 0)
      {
        pq.push(new Node(pair.first, pair.second));
      }
    }
    while (pq.size() != 1)
    {
      Node *left = pq.top();
      pq.pop();
      Node *right = pq.top();
      pq.pop();
      int sum = left->freq + right->freq;
      pq.push(new Node('\0', sum, left, right));
    }
    this->root = pq.top();
  }

  unordered_map<char, string> getEncodings()
  {
    unordered_map<char, string> encodingTable;
    getEncodingsHelper(this->root, "", encodingTable);
    return encodingTable;
  }

  ~HuffmanTreeNaive()
  {
    deleteHelper(this->root);
  }
};