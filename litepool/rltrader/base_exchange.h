#pragma once
#include <map>
#include <vector>
#include "csv_reader.h"
#include "order.h"

namespace Simulator {
    class BaseExchange {
    public:
        // Constructor
        virtual ~BaseExchange() = 0;
        virtual void reset() = 0;
        virtual bool next();
        virtual const DataRow& getObs() const = 0;
        double getDouble(const std::string& name) const;
        std::vector<Order> getFills();

        void cancelBuys();
        void cancelSells();

         const std::map<long, Order>& getBidOrders() const;

         const std::map<long, Order>& getAskOrders() const;

         std::vector<Order> getUnackedOrders() const;

        // Adds a new order to the quote
        void quote(int order_id, OrderSide side, const double& price, const double& amount);
        void market(int order_id, OrderSide side, const double& price, const double& amount);
    };
}
