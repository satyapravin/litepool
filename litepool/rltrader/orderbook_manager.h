#pragma once

#include <atomic>
#include <condition_variable>
#include <chrono>
#include <vector>

#include "orderbook.h"

namespace RLTrader {

    class OrderbookManager {
    private:
        Orderbook book1;
        Orderbook book2;
        std::atomic<Orderbook*> current{&book1};
        std::atomic<bool> changed{false};

        mutable std::mutex cv_mutex;
        mutable std::condition_variable cv;

    public:
        OrderbookManager() = default;

        Orderbook get_current() const;

        bool wait_for_update_timeout(std::chrono::milliseconds timeout);

        void wait_for_update();

        void update(std::vector<double> new_bid_prices,
                    std::vector<double> new_ask_prices,
                    std::vector<double> new_bid_sizes,
                    std::vector<double> new_ask_sizes);

        bool has_update() const;
    };
}