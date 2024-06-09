#pragma once
#include <string>
#include <map>
#include <vector>
#include <stdexcept>
#include "order.h"  
#include "csv_reader.h"

namespace Simulator {
    class Exchange {
    public:
        // Constructor
        Exchange(CsvReader& reader, long delay=300000); // 300 milliseconds delay

        void setDelay(long delay); // override delay

        // Resets the exchange's state
        void reset(int start_pos);

        // Advances to the next row in the data
        bool next();

        // Retrieves the current row of the DataFrame
        const DataRow& getObs() const;

        long long getTimestamp() const;

        double getDouble(const std::string& name) const;

        // Returns executed orders and clears them
        std::vector<Order> getFills();

        // Processes order cancellation for buy orders
        void cancelBuys();

        // Processes order cancellation for sell orders
        void cancelSells();

         const std::map<long, Order>& getBids() const;

         const std::map<long, Order>& getAsks() const;

         std::vector<Order> getUnackedOrders() const;

        // Adds a new order to the quote
        void quote(int order_id, OrderSide side, const double& price, const double& amount);

    private:
        CsvReader dataReader; // reader
        long delay;           // Delay to process timed buffer  
        std::map<long, Order> bid_quotes;  // Active buy orders
        std::map<long, Order> ask_quotes;  // Active sell orders
        std::vector<Order> executions;       // Executed orders
        std::map<long long, std::vector<Order>> timed_buffer;  // Orders waiting for processing

        // cancel orders for a side
        void cancel(std::map<long, Order>& quotes);

        // Executes orders based on current market conditions
        void execute();

        // Adds orders to the buffer
        void addToBuffer(const Order& order);

        // Processes orders that are pending based on their timestamps
        void processPending(const DataRow& obs);
    };
}