#pragma once
#include "base_adaptor.h"
#include "strategy.h"
#include "base_exchange.h"
#include "market_signal_builder.h"
#include "position_signal_builder.h"
#include "trade_signal_builder.h"

namespace RLTrader {
class EnvAdaptor final: public BaseAdaptor{
public:
    EnvAdaptor(Strategy& strat, BaseExchange& exch);
    ~EnvAdaptor() override = default;
    void quote(int buy_spread, int sell_spread, int buy_percent, int sell_percent) override;
    void reset() override;
    bool next() override;
    std::unordered_map<std::string, double>& getInfo() override;
    std::vector<double>& getState() override;
private:;
    void computeState(OrderBook& book);
    void computeInfo(OrderBook& book);
    Strategy& strategy;
    BaseExchange& exchange;
    double max_unrealized_pnl = 0;
    double max_realized_pnl = 0;
    double drawdown = 0;
    long num_trades = 0;
    std::unique_ptr<MarketSignalBuilder> market_builder;
    std::unique_ptr<PositionSignalBuilder> position_builder;
    std::unique_ptr<TradeSignalBuilder> trade_builder;
    std::vector<double> state;
    std::unordered_map<std::string, double> info;
    FixedVector<double, 20> bid_prices;
    FixedVector<double, 20> ask_prices;
    FixedVector<double, 20> bid_sizes;
    FixedVector<double, 20> ask_sizes;
};
}
