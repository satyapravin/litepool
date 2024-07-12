#pragma once
#include "strategy.h"
#include "exchange.h"
#include "market_signal_builder.h"

namespace Simulator {
class EnvAdaptor {
public:
    EnvAdaptor(Strategy& strat, Exchange& exch, uint8_t book_history, uint8_t price_history);
    bool quote(int bid_spread, int ask_spread, const double& buyVolumeAngle, const double& sellVolumeAngle);
    void reset(int time_index, const double& positionAmount, const double& averagePrice);
    PositionInfo fetchPositionData() const;
    std::vector<double> getState();
    bool hasFilled();
private:
    Strategy& strategy;
    Exchange& exchange;
    MarketSignalBuilder builder;
    long numTrades = 0;
};
}
