#include "env_adaptor.h"

using namespace Simulator;

EnvAdaptor::EnvAdaptor(Strategy& strat, Exchange& exch, uint8_t book_history, uint8_t price_history)
           :strategy(strat),
            exchange(exch),
            book_history_lags(book_history),
            price_history_lags(price_history),
            market_builder(std::make_unique<MarketSignalBuilder>(book_history, price_history)) {
}


bool EnvAdaptor::quote(int bid_spread, int ask_spread, const double& buyVolumeAngle, const double& sellVolumeAngle) {
    return this->strategy.quote(bid_spread, ask_spread, buyVolumeAngle, sellVolumeAngle);
}

void EnvAdaptor::reset(int time_index, const double& positionAmount, const double& averagePrice) {
    numTrades = 0;
    return this->strategy.reset(time_index, positionAmount, averagePrice);
    auto ptr = std::make_unique<MarketSignalBuilder>(book_history_lags, price_history_lags);
    market_builder = std::move(ptr);
}

std::vector<double> EnvAdaptor::getState()
{
    auto obs = this->exchange.getObs();
    auto book = Orderbook(obs.data);
    auto market_signals = market_builder->add_book(book);
    PositionInfo info;
    strategy.fetchInfo(info, obs.getBestBidPrice(), obs.getBestAskPrice());
    return market_signals;
}

bool EnvAdaptor::hasFilled(TradeInfo& info) {
    Position& pos = strategy.getPosition();
    auto actualTrades = pos.getNumberOfTrades();
    bool filled = numTrades <  actualTrades;
    numTrades = actualTrades;
    auto& trade_info = pos.getTradeInfo();
    info = trade_info;
    return filled;
}
