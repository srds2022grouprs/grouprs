#pragma once
#include <vector>

template<typename T>
void rotateVector(std::vector<T> & vec) {
    if (vec.size() <= 1) return;
    T first_item = vec[0];
    for (auto i = 0; i < vec.size() - 1; i++) {
        vec[i] = vec[i + 1];
    }
    vec[vec.size() - 1] = first_item;
}