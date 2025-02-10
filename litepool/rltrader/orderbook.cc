#include "orderbook.h"

using namespace RLTrader;


Orderbook::Orderbook(Orderbook&& other) noexcept
          :bid_prices(std::move(other.bid_prices)),
           ask_prices(std::move(other.ask_prices)),
           bid_sizes(std::move(other.bid_sizes)),
           ask_sizes(std::move(other.ask_sizes)) {
}

Orderbook& Orderbook::operator=(Orderbook && other) noexcept {
        if (this != &other) {
            bid_prices = std::move(other.bid_prices);
            ask_prices = std::move(other.ask_prices);
            bid_sizes = std::move(other.bid_sizes);
            ask_sizes = std::move(other.ask_sizes);
        }

        return *this;
}
