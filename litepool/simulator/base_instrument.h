#pragma once
#include <string>

namespace Simulator {
    class BaseInstrument {
    public:
        BaseInstrument(const std::string& aSymbol, const double& aTickSize,
            const double& aMinAmount, const double& aMakerFee, const double& aTakerFee)
            :symbol(aSymbol), tickSize(aTickSize), minAmount(aMinAmount), 
            makerFee(aMakerFee), takerFee(aTakerFee)
        {
        }

        virtual ~BaseInstrument() = default;
        std::string getName() const { return symbol; }
        double getTickSize() const { return tickSize; }
        double getMinAmount() const { return minAmount; }
        virtual double equity(const double& mid, const double& balance, 
                              const double& position, const double& avgPrice, 
                              const double& fee) const = 0;
        virtual double getQtyFromNotional(const double& price, 
                                          const double& notional) const = 0;
        virtual double pnl(const double& qty, const double& entryPrice, 
                           const double& exitPrice) const = 0;
        virtual double fees(const double& qty, const double& price, bool isMaker) const = 0;

    protected:
        std::string symbol;
        double tickSize;
        double minAmount;
        double makerFee;
        double takerFee;
    };
}
