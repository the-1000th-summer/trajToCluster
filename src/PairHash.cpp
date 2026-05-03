#include "PairHash.h"

size_t PairHash::operator()(const std::pair<int, int> &p) const {
    // 使用异或操作符来结合两个整数的哈希值
    //return std::hash<int>{}(p.first) ^ std::hash<int>{}(p.second);
    //return p.first + p.second * p.second;
    return static_cast<size_t>(p.first) + static_cast<size_t>(p.second) * static_cast<size_t>(p.second);
    //return p.first ^ p.second;     // very slow!!!
}
