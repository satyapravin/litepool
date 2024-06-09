#pragma once

#include "base_instrument.h"

namespace Simulator {
    class InverseInstrument : public BaseInstrument {
    public:
        InverseInstrument(const std::string& symbol, const double& tickSize, 
            const double& minAmount, const double& makerFee, const double& takerFee);
        double getQtyFromNotional(const double& price, const double& notional) const override;
        double pnl(const double& qty, const double& entryPrice, const double& exitPrice) const override;
        double equity(const double& mid, const double& balance, const double& position,
                      const double& avgPrice, const double& fee) const override;
        double fees(const double& qty, const double& price, bool isMaker) const override;
    };
}