#pragma once
#include <vector>

template<typename T>
void insert_signals(std::vector<double>& signals, T& repo) {
    size_t num_doubles = sizeof(T) / sizeof(double);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
    auto* start = reinterpret_cast<double*>(&repo);
#pragma GCC diagnostic pop
    double* end = start + num_doubles;
    signals.insert(signals.end(), start, end);
}