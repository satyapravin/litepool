#include "strategy.h"
#include <string>
#include <cmath>
#include <cassert>
#include "orderbook.h"
#include "position_signal_builder.h"

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

void Strategy::quote(const double& buyPercent, const double& sellPercent,
				     const double& buyRatio, const double& sellRatio,
				     const double& spread, const double& skew) {
	assert((buyPercent >= 0 && buyPercent <= 1.0));
	assert((sellPercent >= 0 && sellPercent <= 1.0));
	assert((buyRatio >= 0 && buyRatio <= 1.0));
	assert((sellRatio >= 0 && sellRatio <= 1.0));
	assert((spread >= 0));
	assert((skew >= 0));
	const auto& obs = this->exchange.getObs();
	exchange.cancelBuys();
	exchange.cancelSells();

	this->sendGrid(buyPercent, buyRatio, spread, skew, obs, OrderSide::BUY);
	this->sendGrid(sellPercent, sellRatio, spread, skew, obs, OrderSide::SELL);
}

void Strategy::sendGrid(const double& percent, const double& ratio,
	                    const double& spread, const double& dSkew,
                        const DataRow& obs, OrderSide side) {
	double min_volume = this->instrument.getMinAmount();
	double ref_price = side == OrderSide::BUY ? obs.data.at("bids[0].price") : obs.data.at("asks[0].price");
	double netQty = position.getNetQty();
	double leverage = std::abs(netQty) / position.getInitialBalance();
	double initial_balance = this->position.getInitialBalance() * ref_price * percent;
	if (side == OrderSide::BUY && netQty > 0) initial_balance /= (1 + leverage);
	if (side == OrderSide::SELL && netQty < 0) initial_balance /= (1 + leverage);

	std::string sideStr = side == OrderSide::BUY ? "bids[" : "asks[";

	for (int ii = 0; ii < 5; ++ii) {
		double amount = initial_balance;
		amount = std::round(amount / min_volume) * min_volume;
		if (amount >= min_volume) {
			double price = obs.data.at(sideStr + std::to_string(ii) + "].price");
			this->exchange.quote(++order_id, side, price, amount);
		}
	}
}

bool Strategy::next() {
	bool retval = exchange.next();
	if (retval) {
		auto fills = exchange.getFills();
		for(auto order: fills) {
			position.onFill(order, true);
		}
	}
	return retval;
}