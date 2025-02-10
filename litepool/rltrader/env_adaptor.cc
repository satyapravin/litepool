#include "env_adaptor.h"

#include <iostream>

using namespace RLTrader;

EnvAdaptor::EnvAdaptor(Strategy& strat, BaseExchange& exch, int depth)
           :market_depth(depth),
            strategy(strat),
            exchange(exch),
            market_builder(std::make_unique<MarketSignalBuilder>(depth)),
            position_builder(std::make_unique<PositionSignalBuilder>()),
            trade_builder(std::unique_ptr<TradeSignalBuilder>()) {
}

bool EnvAdaptor::next() {
    state.clear();

    for (int ii=0; ii < 5; ++ii) {
        if(this->exchange.next()) {
            this->strategy.next();
            computeState();
        } else {
            return false;
        }
    }
    
    return true;
}

std::vector<double> EnvAdaptor::getState() {
    return state;
}

void EnvAdaptor::quote(int buy_spread, int sell_spread, int buy_percent, int sell_percent) {
    this->strategy.quote(buy_spread, sell_spread, buy_percent, sell_percent);
}

void EnvAdaptor::reset() {
    max_realized_pnl = 0;
    max_unrealized_pnl = 0;
    drawdown = 0;
    auto market_ptr = std::make_unique<MarketSignalBuilder>(market_depth);
    market_builder = std::move(market_ptr);
    auto position_ptr = std::make_unique<PositionSignalBuilder>();
    position_builder = std::move(position_ptr);
    auto trade_ptr = std::make_unique<TradeSignalBuilder>();
    trade_builder = std::move(trade_ptr);
    this->strategy.reset();
    this->next();
}


std::unordered_map<std::string, double> EnvAdaptor::getInfo() {
    auto book = exchange.getBook();
    auto bid_price = book.bid_prices[0];
    auto ask_price = book.ask_prices[0];
    PositionInfo posInfo =  strategy.getPosition().getPositionInfo(bid_price, ask_price);
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
    retval["trade_count"] = static_cast<double>(tradeInfo.buy_trades + tradeInfo.sell_trades);
    retval["drawdown"] = drawdown;
    retval["buy_amount"] = tradeInfo.buy_amount;
    retval["sell_amount"] = tradeInfo.sell_amount;
    retval["average_buy_price"] = tradeInfo.average_buy_price;
    retval["average_sell_price"] = tradeInfo.average_sell_price;
    retval["fees"] = posInfo.fees;
    return retval;
}

void EnvAdaptor::computeState()
{
    auto book = this->exchange.getBook();
    auto bid_price = book.bid_prices[0];
    auto ask_price = book.ask_prices[0];
    auto market_signals = market_builder->add_book(book);
    PositionInfo position_info = strategy.getPosition().getPositionInfo(book.bid_prices[0], book.ask_prices[0]);
    if (position_info.inventoryPnL > max_unrealized_pnl) max_unrealized_pnl = position_info.inventoryPnL;
    if (position_info.tradingPnL > max_realized_pnl) max_realized_pnl = position_info.tradingPnL;
    auto position_signals = position_builder->add_info(position_info, bid_price, ask_price);
    TradeInfo trade_info = strategy.getPosition().getTradeInfo();
    auto trade_signals = trade_builder->add_trade(trade_info, bid_price, ask_price);
    state.insert(state.end(), market_signals.begin(), market_signals.end());
    state.insert(state.end(), position_signals.begin(), position_signals.end());
    state.insert(state.end(), trade_signals.begin(), trade_signals.end());
}
