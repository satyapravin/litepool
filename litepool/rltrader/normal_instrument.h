#pragma once

#include "base_instrument.h"

namespace Simulator {
    class NormalInstrument : public BaseInstrument {
    public:
        NormalInstrument(const std::string& symbol, const double& tickSize,
            const double& minAmount, const double& makerFee, const double& takerFee);
        [[nodiscard]] double getPositionFromAmount(const double& amount, const double& price) override;
        [[nodiscard]] double getLeverage(const double& amount, const double& equity, const double& price) override;
        [[nodiscard]] double getTradeAmount(const double &amount, const double &refPrice) override;
        [[nodiscard]] double pnl(const double& qty, const double& entryPrice, const double& exitPrice) const override;
        [[nodiscard]] double equity(const double& mid, const double& balance, const double& position,
                      const double& avgPrice, const double& fee) const override;
        [[nodiscard]] double fees(const double& qty, const double& price, bool isMaker) const override;
    };
}