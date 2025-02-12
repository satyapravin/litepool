#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include "orderbook.h"

namespace RLTrader {

class OrderbookManager {
public:
    OrderbookManager() 
        : current(std::make_shared<Orderbook>()), changed(false) {}

    std::shared_ptr<Orderbook> get_current() const {
        return current.load();
    }

    bool wait_for_update_timeout(std::chrono::milliseconds timeout);
    void wait_for_update();
    
    void update(std::vector<double> new_bid_prices, std::vector<double> new_ask_prices,
                std::vector<double> new_bid_sizes, std::vector<double> new_ask_sizes);

    bool has_update() const {
        return changed.load();
    }

private:
    std::atomic<std::shared_ptr<Orderbook>> current;
    std::atomic<bool> changed;
    std::mutex cv_mutex;
    std::condition_variable cv;
};

}  // namespace RLTrader
