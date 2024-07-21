#pragma once
#include "strategy.h"
#include "exchange.h"
#include "market_signal_builder.h"
#include "position_signal_builder.h"
#include "trade_signal_builder.h"

namespace Simulator {
class EnvAdaptor {
public:
    EnvAdaptor(Strategy& strat, Exchange& exch, uint8_t book_history, uint8_t price_history, uint8_t depth);
    void quote(const double& buyPercent, const double& buyRatio,
               const double& sellPercent, const double& sellRatio);
    void reset(int time_index, const double& positionAmount, const double& averagePrice);
    bool next();
    std::unordered_map<std::string, double> getInfo();
    std::vector<double> getState();
    bool is_data_ready() { return this->market_builder->is_data_ready(); }
    bool hasFilled();
    long long getTime();
private:
    std::vector<double> computeState(bool& controlled);

    Strategy& strategy;
    Exchange& exchange;
    uint8_t book_history_lags = 20;
    uint8_t price_history_lags = 20;
    uint8_t depth = 20;
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
