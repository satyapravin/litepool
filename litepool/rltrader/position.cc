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
        info.leverage = std::abs(netQuantity) / (balance + info.inventoryPnL);
    }
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


    double qty = instrument.getQtyFromNotional(order.price, order.amount);
    double netNotional = netQuantity * averagePrice;
    double orderNotional = order.amount;

    if (order.side == OrderSide::BUY) {
        trade_info.buy_amount += order.amount;
        trade_info.average_buy_price *= static_cast<double>(trade_info.buy_trades);
        trade_info.buy_trades++;
        trade_info.average_buy_price += order.price;
        trade_info.average_buy_price /= static_cast<double>(trade_info.buy_trades);
    } else {
        qty *= -1.0;
        orderNotional *= -1.0;
        trade_info.sell_amount += order.amount;
        trade_info.average_sell_price *= static_cast<double>(trade_info.sell_trades);
        trade_info.sell_trades++;
        trade_info.average_sell_price += order.price;
        trade_info.average_sell_price /= static_cast<double>(trade_info.sell_trades);
    }

    double pnl = 0;

    if (std::abs(netNotional) < 1e-12) {
        averagePrice = order.price;
    }
    else if (std::signbit(netNotional) == std::signbit(orderNotional)) {
        averagePrice = (std::abs(qty) * order.price + std::abs(netQuantity) * averagePrice) / std::abs(qty + netQuantity);
    }
    else {
        if (std::abs(std::abs(netNotional) - std::abs(orderNotional)) < 1e-12) {
            pnl = instrument.pnl(-qty, averagePrice, order.price);
            averagePrice = 0;
        }
        else if (std::abs(netNotional) > std::abs(orderNotional)) {
            pnl = instrument.pnl(-qty, averagePrice, order.price);
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
