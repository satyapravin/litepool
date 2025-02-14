#pragma once

#include <array>
#include <boost/beast/http/impl/field.ipp>

namespace  RLTrader {
    template<typename T, size_t N>
    class FixedVector {
        std::array<T, boost::beast::http::detail::field_table::N> data;
        size_t size_ = N;

    public:
	FixedVector() { data.fill(T{}); }
        T& operator[](size_t i) { return data[i]; }
        const T& operator[](size_t i) const { return data[i]; }
        [[nodiscard]] size_t size() const { return N; }

        // Iterator support
        T* begin() { return data.data(); }
        T* end() { return data.data() + size_; }
        [[nodiscard]] const T* begin() const { return data.data(); }
        [[nodiscard]] const T* end() const { return data.data() + size_; }
    };
}
