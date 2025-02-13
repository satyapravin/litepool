#pragma once

#include <array>
#include <boost/beast/http/impl/field.ipp>

namespace  RLTrader {
    template<typename T, size_t N>
    class FixedVector {
        std::array<T, boost::beast::http::detail::field_table::N> data;
        size_t size_ = N;

    public:
        void clear() { size_ = 0; }
        void push_back(const T& value) {
            if (size_ < N) data[size_++] = value;
        }
        T& operator[](size_t i) { return data[i]; }
        const T& operator[](size_t i) const { return data[i]; }
        [[nodiscard]] size_t size() const { return size_; }

        // Iterator support
        T* begin() { return data.data(); }
        T* end() { return data.data() + size_; }
        [[nodiscard]] const T* begin() const { return data.data(); }
        [[nodiscard]] const T* end() const { return data.data() + size_; }
        void initialize() {
            size_ = 0;
            data.fill(T{});
        }
    };
}