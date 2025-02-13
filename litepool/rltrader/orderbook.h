#pragma once
#include "fixed_vector.h"

namespace  RLTrader {
    class alignas(64) OrderBook {
    public:
        static constexpr size_t MAX_LEVELS = 20;

        FixedVector<double, MAX_LEVELS> bid_prices;
        FixedVector<double, MAX_LEVELS> bid_sizes;
        FixedVector<double, MAX_LEVELS> ask_prices;
        FixedVector<double, MAX_LEVELS> ask_sizes;

        void clear() {
            bid_prices.clear();
            bid_sizes.clear();
            ask_prices.clear();
            ask_sizes.clear();
        }

        void initialize() {
            bid_prices.initialize();
            bid_sizes.initialize();
            ask_prices.initialize();
            ask_sizes.initialize();
        }
    };
}
