#pragma once
#include <map>
#include <vector>
#include "order.h"
#include "orderbook.h"

namespace RLTrader {
    class BaseExchange {
    public:
        // Constructor
        virtual ~BaseExchange() = default;

        // Resets the exchange's state
        virtual void reset() = 0;

        // Advances to the next row in the data
        virtual bool next() = 0;

        // build order book from labeled data
        [[nodiscard]] virtual Orderbook orderbook(std::unordered_map<std::string, double> lob) const = 0;

        // fetches the current position from exchange
        virtual void fetchPosition(double& posAmount, double& avgPrice) = 0;

        // Retrieves the current row of the DataFrame
        [[nodiscard]] virtual Orderbook getBook() const = 0;

        // Returns executed orders and clears them
        virtual std::vector<Order> getFills() = 0;

        // Processes order cancellation
        virtual void cancelOrders() = 0;

        [[nodiscard]] virtual const std::map<long, Order>& getBidOrders() const = 0;

        [[nodiscard]] virtual const std::map<long, Order>& getAskOrders() const = 0;

        [[nodiscard]] virtual std::vector<Order> getUnackedOrders() const = 0;

        virtual void quote(int order_id, OrderSide side, const double& price, const double& amount) = 0;

        virtual void market(int order_id, OrderSide side, const double& price, const double& amount) = 0;
    };
}
