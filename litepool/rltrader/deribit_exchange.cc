#include "deribit_exchange.h"

#include <utility>

using namespace RLTrader;

bool DeribitExchange::init = DeribitExchange::initialize();

bool DeribitExchange::initialize() {
    return true;
}

DeribitExchange::DeribitExchange(std::string api_key, std::string api_secret)
    :key(std::move(api_key)), secret(std::move(api_secret))
{
}


