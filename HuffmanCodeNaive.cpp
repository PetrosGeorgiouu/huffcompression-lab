// C++ program to implement huffman coding

#include <iostream>
#include <queue>
#include <unordered_map>
#include <vector>
using namespace std;

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

void deleteTree(Node *root)
{
  if (root != nullptr)
  {
    deleteTree(root->left);
    deleteTree(root->right);
    delete (root);
  }
}

struct compare
{
  bool operator()(Node *l, Node *r)
  {
    return l->freq > r->freq;
  }
};

void obtainHuffmanCode(Node *root, string str,
                       unordered_map<char, string> &huffmanCode)
{
  if (root == nullptr)
    return;
  if (!root->left && !root->right)
  {
    huffmanCode[root->ch] = str;
  }

  obtainHuffmanCode(root->left, str + "0", huffmanCode);
  obtainHuffmanCode(root->right, str + "1", huffmanCode);
}

unordered_map<char, string> buildHuffmanTreeNaive(unordered_map<char, int> freq)
{

  priority_queue<Node *, vector<Node *>, compare> pq;
  for (auto pair : freq)
  {
    pq.push(new Node(pair.first, pair.second));
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
  Node *root = pq.top();

  unordered_map<char, string> huffmanCode;
  obtainHuffmanCode(root, "", huffmanCode);
  deleteTree(root);
  return huffmanCode;
}

string encode(string txt, unordered_map<char, string> huffmanCode)
{
  string str = "";
  for (char ch : txt)
  {
    str += huffmanCode[ch];
  }
  return str;
}