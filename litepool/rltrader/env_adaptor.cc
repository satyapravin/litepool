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
        bool controlled = false;
        state = computeState(controlled);
        return controlled;
    }

    return false;
}

std::vector<double> EnvAdaptor::getState() {
    auto retval =  std::move(this->state);
    this->state.clear();
    return retval;
}

void EnvAdaptor::quote(const double& buyPercent, const double& buyRatio,
                       const double& sellPercent, const double& sellRatio) {
     this->strategy.quote(buyPercent, buyRatio, sellPercent, sellRatio);
}

void EnvAdaptor::reset(int time_index, const double& positionAmount, const double& averagePrice) {
    max_realized_pnl = 0;
    max_unrealized_pnl = 0;
    drawdown = 0;
    long num_trades = 0;
    auto market_ptr = std::make_unique<MarketSignalBuilder>(book_history_lags, price_history_lags, depth);
    market_builder = std::move(market_ptr);
    auto position_ptr = std::make_unique<PositionSignalBuilder>();
    position_builder = std::move(position_ptr);
    auto trade_ptr = std::make_unique<TradeSignalBuilder>();
    trade_builder = std::move(trade_ptr);
    return this->strategy.reset(time_index, positionAmount, averagePrice);
}

long long EnvAdaptor::getTime() {
    return exchange.getTimestamp();
}

std::unordered_map<std::string, double> EnvAdaptor::getInfo() {
    auto obs = exchange.getObs();
    auto bid_price = obs.getBestBidPrice();
    auto ask_price = obs.getBestAskPrice();
    PositionInfo posInfo;
    strategy.fetchInfo(posInfo, bid_price, ask_price);
    auto tradeInfo = strategy.getPosition().getTradeInfo();
    std::unordered_map<std::string, double> retval;
    if (max_unrealized_pnl < posInfo.inventoryPnL) max_unrealized_pnl = posInfo.inventoryPnL;
    if (max_realized_pnl < posInfo.tradingPnL) max_realized_pnl = posInfo.tradingPnL;
    double latest_dd = std::min(posInfo.inventoryPnL - max_unrealized_pnl, 0.0) + std::min(posInfo.tradingPnL - max_realized_pnl, 0.0);
    if (drawdown > latest_dd) drawdown = latest_dd;
    retval["mid_price"] = (bid_price + ask_price) * 0.5;
    retval["balance"] = posInfo.balance;
    retval["unrealized_pnl"] = posInfo.inventoryPnL;
    retval["realized_pnl"] = posInfo.tradingPnL;
    retval["leverage"] = posInfo.leverage;
    retval["trade_count"] = num_trades;
    retval["drawdown"] = drawdown;
    retval["buy_amount"] = tradeInfo.buy_amount;
    retval["sell_amount"] = tradeInfo.sell_amount;
    retval["average_buy_price"] = tradeInfo.average_buy_price;
    retval["average_sell_price"] = tradeInfo.average_sell_price;
    retval["fees"] = posInfo.fees;
    return retval;
}

std::vector<double> EnvAdaptor::computeState(bool& controlled)
{
    auto obs = this->exchange.getObs();
    auto bid_price = obs.getBestBidPrice();
    auto ask_price = obs.getBestAskPrice();
    auto book = Orderbook(obs.data);
    auto market_signals = market_builder->add_book(book);
    PositionInfo position_info;
    if (position_info.inventoryPnL > max_unrealized_pnl) max_unrealized_pnl = position_info.inventoryPnL;
    if (position_info.tradingPnL > max_realized_pnl) max_realized_pnl = position_info.tradingPnL;
    strategy.fetchInfo(position_info, obs.getBestBidPrice(), obs.getBestAskPrice());
    auto position_signals = position_builder->add_info(position_info, bid_price, ask_price);
    TradeInfo trade_info = strategy.getPosition().getTradeInfo();
    auto trade_signals = trade_builder->add_trade(trade_info, bid_price, ask_price);
    std::vector<double> retval;
    retval.reserve(market_signals.size() + position_signals.size() + trade_signals.size());
    retval.insert(retval.end(), market_signals.begin(), market_signals.end());
    retval.insert(retval.end(), position_signals.begin(), position_signals.end());
    retval.insert(retval.end(), trade_signals.begin(), trade_signals.end());
    hasFilled();
    controlled = position_info.leverage < 25;
    return retval;
}

bool EnvAdaptor::hasFilled() {
    Position& pos = strategy.getPosition();
    auto actualTrades = pos.getNumberOfTrades();
    bool filled = num_trades <  actualTrades;
    num_trades = actualTrades;
    return filled;
}
