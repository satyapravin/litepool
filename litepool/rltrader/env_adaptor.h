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
    void quote(int buy_spread, int sell_spread, int buy_percent, int sell_percent);
    void reset(const double& positionAmount, const double& averagePrice);
    bool next();
    std::unordered_map<std::string, double> getInfo();
    std::vector<double> getState();
    [[nodiscard]] long long getTime() const;
private:
    int market_depth;
    void computeState();

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
