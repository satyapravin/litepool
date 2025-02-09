#pragma once
#include "base_exchange.h"


namespace Simulator {
    class CCApiExchange final : public BaseExchange {
    public:
        // initialized labels
        static bool initialize();

        // Constructor
        CCApiExchange(const std::string& exch_name, std::string api_key, std::string api_secret);

        // Generates an orderbook
        [[nodiscard]] Orderbook  orderbook(std::unordered_map<std::string, double> lob) const override;

        // Resets the exchange's state
        void reset() override;

        // Advances to the next row in the data
        bool next() override;

        // fetch dummy zero positions
        void fetchPosition(const std::string& symbol, double& posAmount, double& avgPrice) override;

        // Retrieves the current row of the DataFrame
        [[nodiscard]] Orderbook getBook() const override;

        // Returns executed orders and clears them
        std::vector<Order> getFills() override;

        // Processes order cancellation for buy orders
        void cancelBuys() override;

        // Processes order cancellation for sell orders
        void cancelSells() override;

        [[nodiscard]] const std::map<long, Order>& getBidOrders() const override;

        [[nodiscard]] const std::map<long, Order>& getAskOrders() const override;

        [[nodiscard]] std::vector<Order> getUnackedOrders() const override;

        void quote(int order_id, OrderSide side, const double& price, const double& amount) override;

        void market(int order_id, OrderSide side, const double& price, const double& amount) override;
    };

} // Simulator
