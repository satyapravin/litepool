#include <cmath>
#include <stdexcept>
#include "position.h"

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

void Position::fetchInfo(PositionInfo& info, const double& bidPrice, const double& askPrice) const {
    info.balance = this->balance;
    info.inventoryPnL = this->inventoryPnL(0.5 * (bidPrice + askPrice));
    info.averagePrice = this->averagePrice;
    info.tradingPnL = this->balance - this->initialBalance;
    info.netPosition = this->netQuantity;
    info.leverage = 0;
    
    if (this->averagePrice > instrument.getTickSize()) {
        info.leverage = std::abs(netQuantity) / (netQuantity > 0 ? bidPrice : askPrice) / balance;
    }
}

double Position::inventoryPnL(const double& price) const {
    return this->instrument.pnl(netQuantity, averagePrice, price);
}

void Position::onFill(const Order& order, bool is_maker)
{
    if (order.state != OrderState::FILLED) {
        throw new std::runtime_error("Order state not filled");
    }

    double notional_qty = instrument.getQtyFromNotional(order.price, order.amount);
    double qty = order.amount;
    int totalQtySign = 1;

    double total_notional_qty = 0.0;

    if (netQuantity != 0) {
        total_notional_qty = instrument.getQtyFromNotional(averagePrice, netQuantity);
        if (netQuantity < 0)
            totalQtySign = -1;
    }

    if (order.side != OrderSide::BUY) {
        qty = -qty;
        notional_qty = -notional_qty;
        trade_info.sell_amount += order.amount;
        trade_info.average_sell_price = trade_info.average_sell_price * trade_info.sell_trades + order.price;
        trade_info.sell_trades += 1;
        trade_info.average_sell_price /= trade_info.sell_trades;
    } else {
        trade_info.buy_amount += order.amount;
        trade_info.average_buy_price = trade_info.average_buy_price * trade_info.buy_trades + order.price;
        trade_info.buy_trades += 1;
        trade_info.average_buy_price /= trade_info.buy_trades;
    }

    double pnl = 0.0;

    if (netQuantity == 0) {
        averagePrice = order.price;
    }
    else if (std::signbit(netQuantity) == std::signbit(qty)) {
        if (averagePrice <= 0) throw std::runtime_error("Average price <= 0");
        averagePrice = (std::abs(notional_qty) * order.price + std::abs(total_notional_qty) * averagePrice) /
            (std::abs(notional_qty) + std::abs(total_notional_qty));
    }
    else {
        if (averagePrice <= 0) throw std::runtime_error("Average price <= 0");
        if (std::abs(netQuantity) == std::abs(qty)) {
            pnl = instrument.pnl(netQuantity, averagePrice, order.price);
            averagePrice = 0;
        }
        else if (std::abs(netQuantity) > std::abs(qty)) {
            pnl = instrument.pnl(order.amount * totalQtySign, 
                                 averagePrice, order.price);
        }
        else {
            pnl = instrument.pnl(netQuantity, averagePrice, order.price);
            averagePrice = order.price;
        }
    }

    totalFee += instrument.fees(order.amount, order.price, is_maker);
    numOfTrades++;
    netQuantity += qty;
    balance += pnl;
}
