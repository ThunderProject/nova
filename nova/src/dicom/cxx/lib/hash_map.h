#pragma once
#include "ankerl/unordered_dense.h"

namespace nova {
    template<
        class Key,
        class T,
        class Hash = ankerl::unordered_dense::hash<Key>,
        class KeyEqual = std::equal_to<Key>
    >
    using hash_map = ankerl::unordered_dense::map<Key, T, Hash, KeyEqual>;
}