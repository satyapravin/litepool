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
    void quote(const double& buyVolumeAngle, const double& sellVolumeAngle);
    void reset(int time_index, const double& positionAmount, const double& averagePrice);
    bool next();
    std::vector<std::vector<double>> getState();
    bool is_data_ready() { return this->market_builder->is_data_ready(); }
    bool hasFilled();
private:
    std::vector<double> computeState();

    Strategy& strategy;
    Exchange& exchange;
    uint8_t book_history_lags = 20;
    uint8_t price_history_lags = 20;
    uint8_t depth = 20;
    std::unique_ptr<MarketSignalBuilder> market_builder;
    std::unique_ptr<PositionSignalBuilder> position_builder;
    std::unique_ptr<TradeSignalBuilder> trade_builder;
    long numTrades = 0;
    std::vector<std::vector<double>> state;
};
}
