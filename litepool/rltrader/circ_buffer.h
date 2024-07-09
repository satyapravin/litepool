#pragma once
#include <deque>
#include <memory> // for std::unique_ptr
#include <stdexcept> // for std::out_of_range


namespace Simulator {
template <typename T>
class TemporalBuffer {
public:
    explicit TemporalBuffer(int lags) : size(lags + 1) {
        for (int i = 0; i < size; ++i) {
            buffer.emplace_back(std::make_unique<T>());
        }
    }

    void add(const T& value) {
        std::unique_ptr<T> temp = std::move(buffer.front());
        buffer.pop_front();
        *temp = value;
        buffer.push_back(std::move(temp));
    }

    T& get(int lag) {
        if (lag < 0 || lag >= size) {
            throw std::out_of_range("Lag is out of range");
        }
        return *buffer[buffer.size() - lag - 1];
    }

private:
    int size;
    std::deque<std::unique_ptr<T>> buffer;
};
}
