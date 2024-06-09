#include <cassert>
#include <cmath>
#include "position.h"

using namespace Simulator;

Position::Position(BaseInstrument& instr, const double& aBalance,
    const double& initialQty, const double& initialAvgprice)
    :instrument(instr)
    , initialBalance(aBalance)
    , balance(aBalance)
    , totalQuantity(initialQty)
    , averagePrice(initialAvgprice)
    , totalFee(0)
    , numOfTrades(0)
    , totalNotional(0) {
}

void Position::reset(const double& initialQty, const double& initialPrice) {
    averagePrice = initialPrice;
    totalQuantity = initialQty;
    totalFee = 0.0;
    numOfTrades = 0;
    totalNotional = 0;
    balance = initialBalance;
}

void Position::fetchInfo(PositionInfo& info, const double& bidPrice, const double& askPrice) const {
    info.balance = this->balance;
    info.inventoryPnL = this->inventoryPnL(0.5 * (bidPrice + askPrice));
    info.averagePrice = this->averagePrice;
    info.tradingPnL = this->balance - this->initialBalance;
    info.tradeCount = this->numOfTrades;
    info.leverage = 0;
    
    if (this->averagePrice > instrument.getTickSize()) {
        info.leverage = std::abs(totalQuantity) / (totalQuantity > 0 ? bidPrice : askPrice) / balance;
    }
}

double Position::inventoryPnL(const double& price) const {
    return this->instrument.pnl(totalQuantity, averagePrice, price);
}

void Position::onFill(const Order& order, bool is_maker)
{
    assert(order.state == OrderState::FILLED);
    double notional_qty = instrument.getQtyFromNotional(order.price, order.amount);
    double qty = order.amount;
    int totalQtySign = 1;

    double total_notional_qty = 0.0;

    if (totalQuantity != 0) {
        total_notional_qty = instrument.getQtyFromNotional(averagePrice, totalQuantity);
        if (totalQuantity < 0)
            totalQtySign = -1;
    }

    if (order.side != OrderSide::BUY) {
        qty = -qty;
        notional_qty = -notional_qty;
    }

    double pnl = 0.0;

    if (totalQuantity == 0) {
        averagePrice = order.price;
    }
    else if (std::signbit(totalQuantity) == std::signbit(qty)) {
        assert(averagePrice > 0);
        averagePrice = (std::abs(notional_qty) * order.price + std::abs(total_notional_qty) * averagePrice) /
            (std::abs(notional_qty) + std::abs(total_notional_qty));
    }
    else {
        assert(averagePrice > 0);
        if (std::abs(totalQuantity) == std::abs(qty)) {
            pnl = instrument.pnl(totalQuantity, averagePrice, order.price);
            averagePrice = 0;
        }
        else if (std::abs(totalQuantity) > std::abs(qty)) {
            pnl = instrument.pnl(order.amount * totalQtySign, 
                                 averagePrice, order.price);
        }
        else {
            pnl = instrument.pnl(totalQuantity, averagePrice, order.price);
            averagePrice = order.price;
        }
    }

    totalFee += instrument.fees(order.amount, order.price, is_maker);
    numOfTrades++;
    totalNotional += std::abs(qty);
    totalQuantity += qty;
    balance += pnl;
}