#pragma once
#include "base_exchange.h"
#include "deribit_client.h"
#include "orderbook_manager.h"

namespace RLTrader {
    class DeribitExchange final : public BaseExchange {
    public:
        // Constructor
        DeribitExchange(const std::string& symbol, const std::string& api_key, const std::string& api_secret);

        // Generates an orderbook
        [[nodiscard]] Orderbook  orderbook(std::unordered_map<std::string, double> lob) const override;

        // Resets the exchange's state
        void reset() override;

        // Advances to the next row in the data
        bool next() override;

        // fetch dummy zero positions
        void fetchPosition(double& posAmount, double& avgPrice) override;

        // Retrieves the current row of the DataFrame
        [[nodiscard]] Orderbook getBook() const override;

        // Returns executed orders and clears them
        std::vector<Order> getFills() override;

        // Processes order cancellation
        void cancelOrders() override;

        [[nodiscard]] const std::map<long, Order>& getBidOrders() const override {
            throw std::runtime_error("Unimplemented");
        }

        [[nodiscard]] const std::map<long, Order>& getAskOrders() const override {
            throw std::runtime_error("Unimplemented");
        }

        [[nodiscard]] std::vector<Order> getUnackedOrders() const override {
            throw std::runtime_error("Unimplemented");
        }

        void quote(int order_id, OrderSide side, const double& price, const double& amount) override;

        void market(int order_id, OrderSide side, const double& price, const double& amount) override;
    private:
        void set_callbacks();
        void handle_private_trade_updates (const json& data);
        void handle_order_book_updates (const json& data);
        void handle_order_updates (const json& data);
        void handle_position_updates (const json& data);

        DeribitClient db_client;
        std::map<long, Order> bid_quotes;
        std::map<long, Order> ask_quotes;
        std::vector<Order> executions;

        double position_amount = 0;
        double position_avg_price = 0;
        OrderbookManager book_manager;
    };

} // RLTrader
