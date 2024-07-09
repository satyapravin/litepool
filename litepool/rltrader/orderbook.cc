#include "orderbook.h"

using namespace Simulator;

std::vector<std::string> Orderbook::bid_price_labels(0);
std::vector<std::string> Orderbook::ask_price_labels(0);
std::vector<std::string> Orderbook::bid_size_labels(0);
std::vector<std::string> Orderbook::ask_size_labels(0);
bool Orderbook::init = Orderbook::initialize();

bool Orderbook::initialize() {
    for (int ii = 0; ii < 25; ++ii) {
        std::ostringstream bid_price_lbl;
        bid_price_lbl << "bids[" << ii << "].price";
        std::ostringstream ask_price_lbl;
        ask_price_lbl << "asks[" << ii << "].price";
        std::ostringstream bid_amount_lbl;
        bid_amount_lbl << "bids[" << ii << "].amount";
        std::ostringstream ask_amount_lbl;
        ask_amount_lbl << "asks[" << ii << "].amount";

        Orderbook::bid_price_labels.push_back(bid_price_lbl.str());
        Orderbook::ask_price_labels.push_back(ask_price_lbl.str());
        Orderbook::bid_size_labels.push_back(bid_amount_lbl.str());
        Orderbook::ask_size_labels.push_back(ask_amount_lbl.str());
    }

    return true;
}

Orderbook::Orderbook(std::map<std::string, double> lob){
    for(int ii=0; ii < bid_price_labels.size(); ++ii) {
        if (lob.find(bid_price_labels[ii]) != lob.end()) {
            bid_prices.push_back(lob[Orderbook::bid_price_labels[ii]]);
            ask_prices.push_back(lob[Orderbook::ask_price_labels[ii]]);
            bid_sizes.push_back(lob[Orderbook::bid_size_labels[ii]]);
            ask_sizes.push_back(lob[Orderbook::ask_size_labels[ii]]);
        }
    }
}

Orderbook::Orderbook(Orderbook&& other)
          :bid_prices(std::move(other.bid_prices)),
           ask_prices(std::move(other.ask_prices)),
           bid_sizes(std::move(other.bid_sizes)),
           ask_sizes(std::move(other.ask_sizes)) {
}

Orderbook& Orderbook::operator=(Orderbook && other) {
        if (this != &other) {
            bid_prices = std::move(other.bid_prices);
            ask_prices = std::move(other.ask_prices);
            bid_sizes = std::move(other.bid_sizes);
            ask_sizes = std::move(other.ask_sizes);
        }

        return *this;
}
