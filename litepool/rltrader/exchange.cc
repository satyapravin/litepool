#include "exchange.h"
#include <vector>
#include <cassert>

using namespace Simulator;

Exchange::Exchange(CsvReader& reader, long delay) :dataReader(reader), delay(delay) {
}

void Exchange::setDelay(long delay) {
	this->delay = delay;
}

void Exchange::reset(int startPos) {
	this->dataReader.reset(startPos);
	this->executions.clear();
	this->bid_quotes.clear();
	this->ask_quotes.clear();
	this->timed_buffer.clear();
}

bool Exchange::next() {
	if (this->dataReader.hasNext()) {
		this->dataReader.next();
		this->execute();
		return true;
	}

	return false;
}

const std::map<long, Order>& Exchange::getBidOrders() const {
	return this->bid_quotes;
}

const std::map<long, Order>& Exchange::getAskOrders() const {
	return this->ask_quotes;
}

std::vector<Order> Exchange::getUnackedOrders() const {
	std::vector<Order> retval;
	
	for (auto& [timestamp, orders] : this->timed_buffer) {
		retval.insert(retval.end(), orders.begin(), orders.end());
	}

	return retval;
}

void Exchange::quote(int order_id, OrderSide side, const double& price, const double& amount) {
	Order order;
	order.microSecond = this->dataReader.getTimeStamp();
	order.amount = amount;
	order.orderId = order_id;
	order.price = price;
	order.side = side;
	order.state = OrderState::NEW;
	this->addToBuffer(order);
}

const DataRow& Exchange::getObs() const {
	return this->dataReader.current();
}

long long Exchange::getTimestamp() const {
	return this->dataReader.getTimeStamp();
}

double Exchange::getDouble(const std::string& name) const {
	return this->dataReader.getDouble(name);
}

std::vector<Order> Exchange::getFills() {
	std::vector<Order> retval(this->executions);
	this->executions.clear();
	return retval;
}

void Exchange::cancel(std::map<long, Order>& quotes) {
	for (auto it = quotes.begin(); it != quotes.end(); ++it) {
		if (it->second.state != OrderState::NEW
			&& it->second.state != OrderState::FILLED
			&& it->second.state != OrderState::CANCELLED
			&& it->second.state != OrderState::CANCELLED_ACK) {
			it->second.state = OrderState::CANCELLED;
			it->second.microSecond = this->dataReader.getTimeStamp();
			this->addToBuffer(it->second);
		}
	}
}

void Exchange::processPending(const DataRow& obs) {
	static const double epsilon = 0.0001;
	std::vector<long long> delete_stamps;
	for (auto& [timestamp, orders] : timed_buffer) {
		if (obs.id >= timestamp + delay) {
			for (Order& order : orders) {
				if (order.state == OrderState::NEW) {
					if (order.side == OrderSide::BUY) {
						if (order.price < obs.data.at("asks[0].price") + epsilon) {
							order.state = OrderState::NEW_ACK;
							this->bid_quotes[order.orderId] = order;
						}
					}
					else {
						if (order.price > obs.data.at("bids[0].price") + epsilon) {
							order.state = OrderState::NEW_ACK;
							this->ask_quotes[order.orderId] = order;
						}
					}
				}
				else if (order.state == OrderState::AMEND) {
					if (order.side == OrderSide::BUY) {
						auto& quotes = this->bid_quotes;
					
						if (quotes.find(order.orderId) != quotes.end()) {
							quotes[order.orderId].state = OrderState::AMEND_ACK;
							quotes[order.orderId] = order;
						}
					}
					else {
						auto& quotes = this->ask_quotes;
						if (quotes.find(order.orderId) != quotes.end()) {
							quotes[order.orderId].state = OrderState::AMEND_ACK;
							quotes[order.orderId] = order;
						}
					}
				}
				else if (order.state == OrderState::CANCELLED) {

					if (order.side == OrderSide::BUY) {
						auto it = this->bid_quotes.find(order.orderId);

						if (it != this->bid_quotes.end()) {
							it->second.state = OrderState::CANCELLED_ACK;
							this->bid_quotes.erase(order.orderId);
						}
					}
					else {
						auto it = this->ask_quotes.find(order.orderId);

						if (it != this->ask_quotes.end()) {
							it->second.state = OrderState::CANCELLED_ACK;
							this->ask_quotes.erase(order.orderId);
						}
					}
				}
				else {
					assert(order.state == OrderState::FILLED);
					executions.push_back(order);
				}
			}
		}

		delete_stamps.push_back(timestamp);
	}

	for (auto timestamp : delete_stamps)
		timed_buffer.erase(timestamp);
}

void Exchange::cancelBuys() {
	this->cancel(this->bid_quotes);
}

void Exchange::cancelSells() {
	this->cancel(this->ask_quotes);
}

void Exchange::addToBuffer(const Order& order) {
	if (this->timed_buffer.find(order.microSecond) == this->timed_buffer.end()) {
		this->timed_buffer[order.microSecond] = std::vector<Order>();
	}

	this->timed_buffer[order.microSecond].push_back(order);
}

void Exchange::execute() {
	const DataRow& obs = this->dataReader.current();
	this->processPending(obs);

	std::vector<long> bids_filled;
	std::vector<long> asks_filled;
	static const double epsilon = 0.00001;
	for (auto& [order_id, order] : this->bid_quotes) {
		if (order.side == OrderSide::BUY && order.price > epsilon + this->dataReader.getDouble("asks[0].price")) {
			order.state = OrderState::FILLED;
			bids_filled.push_back(order_id);
			this->addToBuffer(order);
		}
	}


	for(auto& [order_id, order] : this->ask_quotes) {
		if (order.side == OrderSide::SELL && order.price < this->dataReader.getDouble("bids[0].price") - epsilon) {
			order.state = OrderState::FILLED;
			asks_filled.push_back(order_id);
			this->addToBuffer(order);
		}
	}

	for (auto& order_id : bids_filled) {
		this->bid_quotes.erase(order_id);
	}

	for (auto& order_id : asks_filled) {
		this->ask_quotes.erase(order_id);
	}
}
