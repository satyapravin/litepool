#include "orderbook_manager.h"

using namespace RLTrader;

bool OrderbookManager::wait_for_update_timeout(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(cv_mutex);
    bool result = cv.wait_for(lock, timeout, [this]() { return changed.load(); });
    if (result) {
        changed.store(false, std::memory_order_relaxed);
    }
    return result;
}

void OrderbookManager::wait_for_update() {
    std::unique_lock<std::mutex> lock(cv_mutex);
    cv.wait(lock, [this]() { return changed.load(); });
    changed.store(false, std::memory_order_relaxed);
}

void OrderbookManager::update(std::vector<double> new_bid_prices, std::vector<double> new_ask_prices,
                              std::vector<double> new_bid_sizes, std::vector<double> new_ask_sizes) {
    auto next = std::make_shared<Orderbook>();

    next->bid_prices.swap(new_bid_prices);
    next->ask_prices.swap(new_ask_prices);
    next->bid_sizes.swap(new_bid_sizes);
    next->ask_sizes.swap(new_ask_sizes);

    current.store(next, std::memory_order_release);
    changed.store(true, std::memory_order_relaxed);
    cv.notify_all();
}
