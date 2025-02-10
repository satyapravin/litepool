#pragma once
#include <string>

namespace RLTrader {
    class BaseInstrument {
    public:
        BaseInstrument(const std::string& aSymbol, const double& aTickSize, // NOLINT(*-pass-by-value)
            const double& aMinAmount, const double& aMakerFee, const double& aTakerFee)
            :symbol(aSymbol), tickSize(aTickSize), minAmount(aMinAmount), 
            makerFee(aMakerFee), takerFee(aTakerFee)
        {
        }

        virtual ~BaseInstrument() = default;
        [[nodiscard]] std::string getName() const { return symbol; }
        [[nodiscard]] double getTickSize() const { return tickSize; }
        [[nodiscard]] double getMinAmount() const { return minAmount; }
        [[nodiscard]] virtual double getPositionFromAmount(const double& amount, const double& price) = 0;
        [[nodiscard]] virtual double getLeverage(const double& amount, const double& equity, const double& price) = 0;
        [[nodiscard]] virtual double getTradeAmount(const double& amount, const double& refPrice) = 0;
        [[nodiscard]] virtual double equity(const double& mid, const double& balance,
                              const double& position, const double& avgPrice, 
                              const double& fee) const = 0;
        [[nodiscard]] virtual double pnl(const double& qty, const double& entryPrice,
                           const double& exitPrice) const = 0;
        [[nodiscard]] virtual double fees(const double& qty, const double& price, bool isMaker) const = 0;

    protected:
        std::string symbol;
        double tickSize;
        double minAmount;
        double makerFee;
        double takerFee;
    };
}
