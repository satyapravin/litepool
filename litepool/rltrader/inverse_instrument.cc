#include <cmath>
#include "inverse_instrument.h"

using namespace std;
using namespace Simulator;

InverseInstrument::InverseInstrument(const std::string& symbol, const double& tickSize, 
    const double& minAmount, const double& makerFee, const double& takerFee)
    : BaseInstrument(symbol, tickSize, minAmount, makerFee, takerFee) {}

double InverseInstrument::getQtyFromNotional(const double& price, const double& notional) const {
    return notional / price;
}

double InverseInstrument::pnl(const double& qty, const double& entryPrice, const double& exitPrice) const {
    return entryPrice < tickSize ? 0.0 : (qty / entryPrice - qty / exitPrice);
}

double InverseInstrument::equity(const double& mid, const double& balance, const double& position,
    const double& avgPrice, const double& fee) const {
    return balance + this->pnl(position, avgPrice, mid) - fee;
}

double InverseInstrument::fees(const double& qty, const double& price, bool isMaker) const {
    if (isMaker)
    {
        return abs(qty) * this->makerFee / price;
    }
    else {
        return abs(qty) * this->takerFee / price;
    }
}