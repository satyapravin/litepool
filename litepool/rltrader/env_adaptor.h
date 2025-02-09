#pragma once
#include "base_adaptor.h"
#include "strategy.h"
#include "sim_exchange.h"
#include "market_signal_builder.h"
#include "position_signal_builder.h"
#include "trade_signal_builder.h"

namespace Simulator {
class EnvAdaptor final: public BaseAdaptor{
public:
    EnvAdaptor(Strategy& strat, SimExchange& exch, int depth);
    ~EnvAdaptor() override = default;
    void quote(int buy_spread, int sell_spread, int buy_percent, int sell_percent) override;
    void reset(const double& positionAmount, const double& averagePrice) override;
    bool next() override;
    std::unordered_map<std::string, double> getInfo() override;
    std::vector<double> getState() override;
private:
    int market_depth;
    void computeState();

    Strategy& strategy;
    SimExchange& exchange;
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
