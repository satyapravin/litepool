#include "env_adaptor.h"

#include <iostream>

using namespace RLTrader;

EnvAdaptor::EnvAdaptor(Strategy& strat, BaseExchange& exch):
            strategy(strat),
            exchange(exch),
            market_builder(std::make_unique<MarketSignalBuilder>()),
            position_builder(std::make_unique<PositionSignalBuilder>()),
            trade_builder(std::unique_ptr<TradeSignalBuilder>()),
            bid_prices(), ask_prices(), bid_sizes(), ask_sizes() {
}

bool EnvAdaptor::next() {
    state.clear();

    for (int ii=0; ii < 5; ++ii) {
        OrderBook book;
        size_t read_slot;
        if(this->exchange.next_read(read_slot, book)) {
            this->strategy.next();
            computeState(book);
            std::ranges::copy(bid_prices, book.bid_prices.begin());
            std::ranges::copy(ask_prices, book.ask_prices.begin());
            std::ranges::copy(bid_sizes, book.bid_sizes.begin());
            std::ranges::copy(ask_sizes, book.ask_sizes.begin());
            this->exchange.done_read(read_slot);
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
    this->strategy.quote(buy_spread, sell_spread, buy_percent, sell_percent, bid_prices, ask_prices);
}

void EnvAdaptor::reset() {
    max_realized_pnl = 0;
    max_unrealized_pnl = 0;
    drawdown = 0;
    auto market_ptr = std::make_unique<MarketSignalBuilder>();
    market_builder = std::move(market_ptr);
    auto position_ptr = std::make_unique<PositionSignalBuilder>();
    position_builder = std::move(position_ptr);
    auto trade_ptr = std::make_unique<TradeSignalBuilder>();
    trade_builder = std::move(trade_ptr);
    this->strategy.reset();
    this->state.assign(490, 0);
}


std::unordered_map<std::string, double> EnvAdaptor::getInfo() {
   return info;
}

void EnvAdaptor::computeInfo(OrderBook &book) {
    auto bid_price = book.bid_prices[0];
    auto ask_price = book.ask_prices[0];
    PositionInfo posInfo =  strategy.getPosition().getPositionInfo(bid_price, ask_price);
    auto tradeInfo = strategy.getPosition().getTradeInfo();

    if (max_unrealized_pnl < posInfo.inventoryPnL) max_unrealized_pnl = posInfo.inventoryPnL;
    if (max_realized_pnl < posInfo.tradingPnL) max_realized_pnl = posInfo.tradingPnL;
    double latest_dd = std::min(posInfo.inventoryPnL - max_unrealized_pnl, 0.0) + std::min(posInfo.tradingPnL - max_realized_pnl, 0.0);
    if (drawdown > latest_dd) drawdown = latest_dd;
    info["mid_price"] = (bid_price + ask_price) * 0.5;
    info["balance"] = posInfo.balance;
    info["unrealized_pnl"] = posInfo.inventoryPnL;
    info["realized_pnl"] = posInfo.tradingPnL;
    info["leverage"] = posInfo.leverage;
    info["trade_count"] = static_cast<double>(tradeInfo.buy_trades + tradeInfo.sell_trades);
    info["drawdown"] = drawdown;
    info["buy_amount"] = tradeInfo.buy_amount;
    info["sell_amount"] = tradeInfo.sell_amount;
    info["average_buy_price"] = tradeInfo.average_buy_price;
    info["average_sell_price"] = tradeInfo.average_sell_price;
    info["fees"] = posInfo.fees;
}


void EnvAdaptor::computeState(OrderBook& book)
{
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
    computeInfo(book);
}
