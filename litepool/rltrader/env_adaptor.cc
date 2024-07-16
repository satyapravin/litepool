#include "env_adaptor.h"

using namespace Simulator;

EnvAdaptor::EnvAdaptor(Strategy& strat, Exchange& exch, uint8_t book_history, uint8_t price_history, uint8_t depth)
           :strategy(strat),
            exchange(exch),
            book_history_lags(book_history),
            price_history_lags(price_history),
            depth(depth),
            market_builder(std::make_unique<MarketSignalBuilder>(book_history, price_history, depth)),
            position_builder(std::make_unique<PositionSignalBuilder>()),
            trade_builder(std::unique_ptr<TradeSignalBuilder>()) {
}

bool EnvAdaptor::next() {
    if(this->strategy.next()) {
        std::vector<double> next_state = computeState();
        state.push_back(next_state);
        return true;
    }

    return false;
}

std::vector<std::vector<double>> EnvAdaptor::getState() {
    auto retval =  std::move(this->state);
    this->state.clear();
    return retval;
}

void EnvAdaptor::quote(const double& buyVolumeAngle, const double& sellVolumeAngle) {
     this->strategy.quote(buyVolumeAngle, sellVolumeAngle);
}

void EnvAdaptor::reset(int time_index, const double& positionAmount, const double& averagePrice) {
    numTrades = 0;
    auto market_ptr = std::make_unique<MarketSignalBuilder>(book_history_lags, price_history_lags, depth);
    market_builder = std::move(market_ptr);
    auto position_ptr = std::make_unique<PositionSignalBuilder>();
    position_builder = std::move(position_ptr);
    auto trade_ptr = std::make_unique<TradeSignalBuilder>();
    trade_builder = std::move(trade_ptr);
    return this->strategy.reset(time_index, positionAmount, averagePrice);
}

std::vector<double> EnvAdaptor::computeState()
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
    std::vector<double> retval;
    retval.reserve(market_signals.size() + position_signals.size() + trade_signals.size());
    retval.insert(retval.end(), market_signals.begin(), market_signals.end());
    retval.insert(retval.end(), position_signals.begin(), position_signals.end());
    retval.insert(retval.end(), trade_signals.begin(), trade_signals.end());
    return retval;
}

bool EnvAdaptor::hasFilled() {
    Position& pos = strategy.getPosition();
    auto actualTrades = pos.getNumberOfTrades();
    bool filled = numTrades <  actualTrades;
    numTrades = actualTrades;
    return filled;
}
