#include <cmath>
#include "normal_instrument.h"

using namespace std;
using namespace RLTrader;

NormalInstrument::NormalInstrument(const std::string& symbol, const double& tickSize,
    const double& minAmount, const double& makerFee, const double& takerFee)
    : BaseInstrument(symbol, tickSize, minAmount, makerFee, takerFee) {}

double NormalInstrument::getPositionFromAmount(const double& amount, const double& price) {
    return amount * price;
}

double NormalInstrument::getLeverage(const double& amount, const double& equity, const double& price) {
    return amount * price / equity;
}
double NormalInstrument::getTradeAmount(const double &amount, const double &refPrice) {
    return std::round(amount / refPrice / minAmount) * minAmount;
}

double NormalInstrument::pnl(const double& qty, const double& entryPrice, const double& exitPrice) const {
    return entryPrice < tickSize ? 0.0 : qty * (exitPrice - entryPrice);
}

double NormalInstrument::equity(const double& mid, const double& balance, const double& position,
    const double& avgPrice, const double& fee) const {
    return balance + this->pnl(position, avgPrice, mid) - fee;
}

double NormalInstrument::fees(const double& qty, const double& price, bool isMaker) const {
    if (isMaker)
    {
        return abs(qty) * this->makerFee * price;
    }
    else {
        return abs(qty) * this->takerFee * price;
    }
}