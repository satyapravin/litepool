#include "strategy.h"
#include <string>
#include <cmath>
#include <cassert>
#include "orderbook.h"

using namespace Simulator;

Strategy::Strategy(BaseInstrument& instr, Exchange& exch, const double& balance,
	               const double& pos_amount, const double& avg_price,
	               const double& areaFactor, int maxTicks)
	:instrument(instr), exchange(exch),
	 position(instr, balance, pos_amount, avg_price),
	 order_id(0), maxFactor(areaFactor), max_ticks(maxTicks) {

	assert(max_ticks >= 5);
}

void Strategy::reset(int time_index, const double& position_amount, const double& avg_price) {
	this->exchange.reset(time_index);
	this->position.reset(position_amount, avg_price);
	this->order_id = 0;
}

void Strategy::quote(const double& buyPercent, const double& buyRatio,
	                 const double& sellPercent, const double& sellRatio) {
	assert((buyPercent >= 0 && buyPercent <= 0.1));
	assert((sellPercent >= 0 && sellPercent <= 0.1));
	assert((buyRatio >= 0.1 && buyRatio <= 0.99));
	assert((sellRatio >= 0.1 && sellRatio <= 0.99));
	const auto& obs = this->exchange.getObs();
	this->sendGrid(buyPercent, buyRatio, obs, OrderSide::BUY);
	this->sendGrid(sellPercent, sellRatio, obs, OrderSide::SELL);
}

void Strategy::sendGrid(const double& percent, const double& ratio, const DataRow& obs, OrderSide side) {
	double min_volume = this->instrument.getMinAmount();
	double ref_price = side == OrderSide::BUY ? obs.data.at("bids[0].price") : obs.data.at("asks[0].price");
	double initial_balance = this->position.getInitialBalance() * ref_price * percent;
	std::string sideStr = side == OrderSide::BUY ? "bids[" : "asks[";

	std::vector<double> amounts;
	double base_amount = initial_balance * (1.0 - ratio) / (1.0 - std::pow(ratio, 5));

	for (int ii = 0; ii < 5; ++ii) {
		double amount = base_amount * std::pow(ratio, 5 - (ii+1));
		amount = std::round(amount / min_volume) * min_volume;
		if (amount >= min_volume) {
			double price = obs.data.at(sideStr + std::to_string(ii) + "].price");
			this->exchange.quote(++order_id, side, price, amount);
		}
	}
}

bool Strategy::next() {
	bool retval = exchange.next();
	auto fills = exchange.getFills();
	for(auto order: fills) {
		position.onFill(order, true);
	}
	return retval;
}

void Strategy::fetchInfo(PositionInfo& info, const double& bidPrice, const double& askPrice) {
	return position.fetchInfo(info, bidPrice, askPrice);
}
