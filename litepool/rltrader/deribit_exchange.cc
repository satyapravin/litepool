#include "deribit_exchange.h"

using namespace RLTrader;


DeribitExchange::DeribitExchange(const std::string& symbol, const std::string& api_key, const std::string& api_secret)
    :db_client(api_key, api_secret, symbol)
{
}

Orderbook DeribitExchange::orderbook(std::unordered_map<std::string, double> lob) const {
    throw std::runtime_error("cannot construct orderbook from unordered_map in prod exchange");
}

void DeribitExchange::reset() {
    db_client.stop();
    this->executions.clear();
    this->bid_quotes.clear();
    this->ask_quotes.clear();
    this->set_callbacks();
    db_client.start();
}

void DeribitExchange::set_callbacks() {
    this->db_client.set_private_trade_cb([this] (const json& data) { handle_private_trade_updates(data); });
    this->db_client.set_orderbook_cb([this] (const json& data) { handle_order_book_updates(data); });
    this->db_client.set_order_cb([this] (const json& data) { handle_order_updates(data);});
    this->db_client.set_position_cb([this] (const json& data) { handle_position_updates(data);});
}

void DeribitExchange::handle_private_trade_updates (const json& data) {

}

void DeribitExchange::handle_order_book_updates (const json& data) {

}

void DeribitExchange::handle_order_updates (const json& data) {

}

void DeribitExchange::handle_position_updates (const json& data) {

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