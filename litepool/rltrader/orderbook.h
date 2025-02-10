#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>

namespace RLTrader {
class Orderbook {
public:
    Orderbook() = default;

    Orderbook(Orderbook&& other) noexcept ;

    Orderbook& operator=(Orderbook && other) noexcept ;

    Orderbook(const Orderbook&) = default;
    Orderbook& operator=(const Orderbook&) = default;


    std::vector<double> bid_prices;
    std::vector<double> ask_prices;
    std::vector<double> bid_sizes;
    std::vector<double> ask_sizes;
};
}
