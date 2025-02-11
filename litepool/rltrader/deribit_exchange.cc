#include "deribit_exchange.h"

#include <iostream>

using namespace RLTrader;


DeribitExchange::DeribitExchange(const std::string& symbol, const std::string& api_key, const std::string& api_secret)
    :db_client(api_key, api_secret, symbol), symbol(symbol), orders_count(0)
{
}

Orderbook DeribitExchange::orderbook(std::unordered_map<std::string, double> lob) const {
    throw std::runtime_error("cannot construct orderbook from unordered_map in prod exchange");
}

void DeribitExchange::reset() {
    db_client.stop();
    std::lock_guard<std::mutex> lock(this->fill_mutex);
    this->executions.clear();
    this->set_callbacks();
    db_client.start();
}

void DeribitExchange::set_callbacks() {
    this->db_client.set_private_trade_cb([this] (const json& data) { handle_private_trade_updates(data); });
    this->db_client.set_orderbook_cb([this] (const json& data) { handle_order_book_updates(data); });
    this->db_client.set_order_cb([this] (const json& data) { handle_order_updates(data);});
    this->db_client.set_position_cb([this] (const json& data) { handle_position_updates(data);});
}

void DeribitExchange::handle_private_trade_updates (const json& json_array) {
    const auto& data = json_array[0];
    std::cout << data << std::endl;
    if (data["instrument_name"] == this->symbol) {
        Order order;
        order.amount = data["amount"];
        order.is_taker = false;
        order.price = data["price"];
        order.side = data["direction"] == "buy" ? OrderSide::BUY : OrderSide::SELL;
        order.state = OrderState::FILLED;
        order.orderId = data["order_id"];
        order.microSecond = data["timestamp"];
        std::lock_guard<std::mutex> lock(this->fill_mutex);
        this->executions.push_back(order);
    }
}

void DeribitExchange::handle_order_book_updates (const json& data) {
    std::vector<double> bid_prices;
    std::vector<double> bid_sizes;
    std::vector<double> ask_prices;
    std::vector<double> ask_sizes;


    // Extract bids
    for (const auto& bid : data["bids"]) {
        bid_prices.push_back(bid[0].get<double>());
        bid_sizes.push_back(bid[1].get<double>());
    }

    // Extract asks
    for (const auto& ask : data["asks"]) {
        ask_prices.push_back(ask[0].get<double>());
        ask_sizes.push_back(ask[1].get<double>());
    }

    this->book_manager.update(std::move(bid_prices), std::move(ask_prices),
                              std::move(bid_sizes), std::move(ask_sizes));
}

void DeribitExchange::handle_order_updates (const json& data) {
    ++this->orders_count;
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void DeribitExchange::handle_position_updates (const json& data) { // NOLINT(*-convert-member-functions-to-static)
    std::cout << data << std::endl;
}

bool DeribitExchange::next() {
    this->book_manager.wait_for_update();
    return true;
}

void DeribitExchange::fetchPosition(double &posAmount, double &avgPrice) {
    posAmount = this->position_amount;
    avgPrice = this->position_avg_price;
}

void DeribitExchange::cancelOrders() {
    db_client.cancel_all_orders();
}

Orderbook DeribitExchange::getBook() const {
    return book_manager.get_current();
}

std::vector<Order> DeribitExchange::getFills() {
    std::lock_guard<std::mutex> lock(this->fill_mutex);
    std::vector<Order> fills(this->executions);
    this->executions.clear();
    return fills;
}

void DeribitExchange::quote(std::string order_id, OrderSide side, const double& price, const double& amount) {
    std::string sidestr = side == OrderSide::BUY ? "buy" : "sell";
    this->db_client.place_order(sidestr, price, amount, "limit");
}

void DeribitExchange::market(std::string order_id, OrderSide side, const double& price, const double& amount) {
    std::string sidestr = side == OrderSide::BUY ? "buy" : "sell";
    this->db_client.place_order(sidestr, price, amount, "market");
}
