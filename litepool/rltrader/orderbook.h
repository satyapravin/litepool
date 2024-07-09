#pragma once

#include <map>
#include <vector>
#include <string>
#include <sstream>

namespace Simulator {
class Orderbook {
private:
    static std::vector<std::string> ask_price_labels;
    static std::vector<std::string> bid_price_labels;
    static std::vector<std::string> ask_size_labels;
    static std::vector<std::string> bid_size_labels;
    static bool init;
public:
    static bool initialize();
    Orderbook() {}
    Orderbook(std::map<std::string, double> lob);

    Orderbook(Orderbook&& other);

    Orderbook& operator=(Orderbook && other);

    Orderbook(const Orderbook&) = default;
    Orderbook& operator=(const Orderbook&) = default;


    std::vector<double> bid_prices;
    std::vector<double> ask_prices;
    std::vector<double> bid_sizes;
    std::vector<double> ask_sizes;
};
}
