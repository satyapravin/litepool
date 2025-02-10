#include "orderbook_manager.h"

using namespace RLTrader;

Orderbook OrderbookManager::get_current() const {
    return *current.load();
}

bool OrderbookManager::wait_for_update_timeout(std::chrono::milliseconds timeout) {  // removed const
    std::unique_lock<std::mutex> lock(cv_mutex);
    const bool result = cv.wait_for(lock, timeout,
        [this]() { return changed.load(); });
    if (result) {
        changed.store(false);
    }
    return result;
}

void OrderbookManager::wait_for_update() {  // removed const
    std::unique_lock<std::mutex> lock(cv_mutex);
    cv.wait(lock, [this]() { return changed.load(); });
    changed.store(false);
}

void OrderbookManager::update(const std::vector<double>& new_bid_prices,
    const std::vector<double>& new_ask_prices,
    const std::vector<double>& new_bid_sizes,
    const std::vector<double>& new_ask_sizes) {

    Orderbook* curr = current.load();
    Orderbook* next = (curr == &book1) ? &book2 : &book1;

    next->bid_prices = new_bid_prices;
    next->ask_prices = new_ask_prices;
    next->bid_sizes = new_bid_sizes;
    next->ask_sizes = new_ask_sizes;

    current.store(next);
    changed.store(true);
    cv.notify_all();
}

bool OrderbookManager::has_update() const {
    return changed.load();
}