#include "env_adaptor.h"

using namespace Simulator;

EnvAdaptor::EnvAdaptor(Strategy& strat, Exchange& exch, uint8_t book_history, uint8_t price_history)
           :strategy(strat),
            exchange(exch),
            book_history_lags(book_history),
            price_history_lags(price_history),
            market_builder(std::make_unique<MarketSignalBuilder>(book_history, price_history)),
            position_builder(std::make_unique<PositionSignalBuilder>()),
            trade_builder(std::unique_ptr<TradeSignalBuilder>()) {
}


bool EnvAdaptor::quote(int bid_spread, int ask_spread, const double& buyVolumeAngle, const double& sellVolumeAngle) {
    return this->strategy.quote(bid_spread, ask_spread, buyVolumeAngle, sellVolumeAngle);
}

void EnvAdaptor::reset(int time_index, const double& positionAmount, const double& averagePrice) {
    numTrades = 0;
    return this->strategy.reset(time_index, positionAmount, averagePrice);
    auto market_ptr = std::make_unique<MarketSignalBuilder>(book_history_lags, price_history_lags);
    market_builder = std::move(market_ptr);
    auto position_ptr = std::make_unique<PositionSignalBuilder>();
    position_builder = std::move(position_ptr);
    auto trade_ptr = std::make_unique<TradeSignalBuilder>();
    trade_builder = std::move(trade_builder);
}

std::vector<double> EnvAdaptor::getState()
{
    auto obs = this->exchange.getObs();
    auto bid_price = obs.getBestBidPrice();
    auto ask_price = obs.getBestAskPrice();
    auto book = Orderbook(obs.data);
    auto market_signals = market_builder->add_book(book);
    PositionInfo position_info;
    strategy.fetchInfo(position_info, obs.getBestBidPrice(), obs.getBestAskPrice());
    auto position_signals = position_builder->add_info(position_info, bid_price, ask_price);
    TradeInfo trade_info = strategy.getPosition().getTradeInfo();
    auto trade_signals = trade_builder->add_trade(trade_info, bid_price, ask_price);
    std::vector<double> retval(market_signals.size() + position_signals.size() + trade_signals.size());
    retval.insert(retval.end(), market_signals.begin(), market_signals.end());
    retval.insert(retval.end(), position_signals.begin(), position_signals.end());
    retval.insert(retval.end(), trade_signals.begin(), trade_signals.end());
    return market_signals;
}

bool EnvAdaptor::hasFilled() {
    Position& pos = strategy.getPosition();
    auto actualTrades = pos.getNumberOfTrades();
    bool filled = numTrades <  actualTrades;
    numTrades = actualTrades;
    return filled;
}
