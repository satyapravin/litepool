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

void OrderbookManager::update(std::vector<double> new_bid_prices, std::vector<double> new_ask_prices,
                              std::vector<double> new_bid_sizes, std::vector<double> new_ask_sizes) {

    Orderbook* curr = current.load();
    Orderbook* next = (curr == &book1) ? &book2 : &book1;

    next->bid_prices = std::move(new_bid_prices);
    next->ask_prices = std::move(new_ask_prices);
    next->bid_sizes = std::move(new_bid_sizes);
    next->ask_sizes = std::move(new_ask_sizes);

    current.store(next);
    changed.store(true);
    cv.notify_all();
}

bool OrderbookManager::has_update() const {
    return changed.load();
}