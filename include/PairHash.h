#pragma once
#include <utility>
#include <cstddef>

struct PairHash {
    size_t operator()(const std::pair<int, int> &p) const;
};

