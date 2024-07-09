#pragma once
#include "strategy.h"
#include "exchange.h"

namespace Simulator {
class EnvAdaptor {
public:
    EnvAdaptor(Strategy& strat, Exchange& exch);
    bool quote(int bid_spread, int ask_spread, const double& buyVolumeAngle, const double& sellVolumeAngle);
    void reset(int time_index, const double& positionAmount, const double& averagePrice);
    PositionInfo fetchPositionData() const;
    std::vector<double> getState() const;
    bool hasFilled() const;
private:
    Strategy& strategy;
    Exchange& exchange;
    long numTrades = 0;
};
}
