#include <cmath>
#include <stdexcept>
#include "position.h"

#include <iostream>
using namespace Simulator;

Position::Position(BaseInstrument& instr, const double& aBalance,
    const double& initialQty, const double& initialAvgprice)
    :instrument(instr)
    , averagePrice(initialAvgprice)
    , netQuantity(initialQty)
    , totalFee(0)
    , numOfTrades(0)
    , initialBalance(aBalance)
    , balance(aBalance){
}

void Position::reset(const double& initialQty, const double& initialPrice) {
    averagePrice = initialPrice;
    netQuantity = initialQty;
    totalFee = 0.0;
    numOfTrades = 0;
    balance = initialBalance;
    trade_info.buy_trades = 0;
    trade_info.sell_trades = 0;
    trade_info.buy_amount = 0;
    trade_info.sell_amount = 0;
    trade_info.average_buy_price = 0;
    trade_info.average_sell_price = 0;
}

PositionInfo Position::getPositionInfo(const double& bidPrice, const double& askPrice) const {
    PositionInfo info;
    info.balance = this->balance;
    info.inventoryPnL = this->inventoryPnL(0.5 * (bidPrice + askPrice));
    info.averagePrice = this->averagePrice;
    info.tradingPnL = this->balance - this->initialBalance;
    info.netPosition = this->netQuantity;
    info.leverage = 0;
    info.fees = totalFee;
    
    if (this->averagePrice > instrument.getTickSize()) {
        info.leverage = std::abs(netQuantity) / (balance + info.inventoryPnL);
    }

    return info;
}

double Position::inventoryPnL(const double& price) const {
    return this->instrument.pnl(netQuantity * averagePrice, averagePrice, price);
}

void Position::onFill(const Order& order, bool is_maker)
{
    if (order.state != OrderState::FILLED) {
        throw std::runtime_error("Order state not filled");
    }

    if (order.amount < 0) {
        throw std::runtime_error("Invalid order amount");
    }


    if (order.side == OrderSide::BUY) {
        trade_info.average_buy_price *= trade_info.buy_amount;
        trade_info.buy_trades++;
        trade_info.average_buy_price += order.price * order.amount;
        trade_info.buy_amount += order.amount;
        trade_info.average_buy_price /= trade_info.buy_amount;
    } else {
        trade_info.average_sell_price *= trade_info.sell_amount;
        trade_info.sell_trades++;
        trade_info.average_sell_price += order.price * order.amount;
        trade_info.sell_amount += order.amount;
        trade_info.average_sell_price /= trade_info.sell_amount;
    }

    double pnl = 0;
    double orderQty = order.amount / order.price;
    double sideSign = order.side == OrderSide::BUY ? 1.0 : -1.0;
    double netNotional = netQuantity * averagePrice;

    if (std::abs(netNotional) < 1e-3) {
        averagePrice = order.price;
        netQuantity = orderQty * sideSign;
    }
    else if (std::signbit(netNotional) == std::signbit(sideSign)) {
        netQuantity += orderQty * sideSign;
        averagePrice = (order.amount + std::abs(netQuantity) * averagePrice) / (orderQty + std::abs(netQuantity));
    }
    else {
        if (orderQty >= std::abs(netQuantity)) {
            pnl = instrument.pnl(netQuantity * averagePrice, averagePrice, order.price);
            averagePrice = order.price;
            netQuantity += orderQty * sideSign;
        }
        else {
            pnl = instrument.pnl(order.amount * -sideSign, averagePrice, order.price);
            netQuantity += orderQty * sideSign;
        }
    }


    totalFee += instrument.fees(order.amount, order.price, !order.is_taker);
    numOfTrades++;
    balance += pnl;

    if (order.is_taker) std::cout << "pnl " << pnl << std::endl;
}
