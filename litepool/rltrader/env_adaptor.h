#pragma once
#include "strategy.h"
#include "exchange.h"
#include "market_signal_builder.h"
#include "position_signal_builder.h"
#include "trade_signal_builder.h"

namespace Simulator {
class EnvAdaptor {
public:
    EnvAdaptor(Strategy& strat, Exchange& exch, int depth);
    void quote(int buy_spread, int sell_spred, int buy_percent, int sell_percent);
    void reset(int time_index, const double& positionAmount, const double& averagePrice);
    bool next();
    std::unordered_map<std::string, double> getInfo();
    std::vector<double> getState();
    long long getTime();
private:
    int market_depth;
    std::vector<double> computeState();

    Strategy& strategy;
    Exchange& exchange;
    double max_unrealized_pnl = 0;
    double max_realized_pnl = 0;
    double drawdown = 0;
    long num_trades = 0;
    std::unique_ptr<MarketSignalBuilder> market_builder;
    std::unique_ptr<PositionSignalBuilder> position_builder;
    std::unique_ptr<TradeSignalBuilder> trade_builder;
    std::vector<double> state;
};
}
