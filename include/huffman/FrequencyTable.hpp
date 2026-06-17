#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

using namespace std;

unordered_map<char, uint64_t> findFrequenciesNaive(const string &txtPath);