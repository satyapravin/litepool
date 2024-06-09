#pragma once

#include <string>

namespace Simulator {
    enum class OrderState {
        NEW = 1,
        NEW_ACK = 2,
        AMEND = 3,
        AMEND_ACK = 4,
        CANCELLED = 5,
        CANCELLED_ACK = 6,
        FILLED = 7
    };

    enum OrderSide {
        BUY = 1,
        SELL = 2
    };

    struct Order
    {
        int orderId;
        OrderSide side;
        double price;
        double amount;
        OrderState state;
        long long microSecond;
    };
}