#include "env_adaptor.h"

using namespace Simulator;

EnvAdaptor::EnvAdaptor(Strategy& strat, Exchange& exch, uint8_t book_history, uint8_t price_history)
           :strategy(strat),
            exchange(exch),
            builder(book_history, price_history) {
}


bool EnvAdaptor::quote(int bid_spread, int ask_spread, const double& buyVolumeAngle, const double& sellVolumeAngle) {
    return this->strategy.quote(bid_spread, ask_spread, buyVolumeAngle, sellVolumeAngle);
}

void EnvAdaptor::reset(int time_index, const double& positionAmount, const double& averagePrice) {
    numTrades = 0;
    return this->strategy.reset(time_index, positionAmount, averagePrice);
}

PositionInfo EnvAdaptor::fetchPositionData() const {
    PositionInfo info;
    auto obs = exchange.getObs();
    strategy.fetchInfo(info, obs.getBestBidPrice(), obs.getBestAskPrice());
    return info;
}

std::vector<double> EnvAdaptor::getState()
{
    auto obs = this->exchange.getObs();
    auto book = Orderbook(obs.data);
    auto market_signals = builder.add_book(book);
    return market_signals;
}

bool EnvAdaptor::hasFilled() {
    auto actualTrades = strategy.numOfTrades();
    bool filled = numTrades <  actualTrades;
    numTrades = actualTrades;
    return filled;
}
