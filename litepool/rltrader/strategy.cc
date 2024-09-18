#include "strategy.h"
#include <string>
#include <cmath>
#include <cassert>
#include "orderbook.h"
#include "position_signal_builder.h"
#include "trade_signal_builder.h"

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

void Strategy::quote(int buy_spread, int sell_spread, int buy_percent, int sell_percent) {
	const auto& obs = this->exchange.getObs();
	exchange.cancelBuys();
	exchange.cancelSells();
	double buy_volume = position.getInitialBalance() * buy_percent / 100.0;
	double sell_volume = position.getInitialBalance() * sell_percent / 100.0;
	this->sendGrid(buy_spread, buy_volume, obs, OrderSide::BUY);
	this->sendGrid(sell_spread, sell_volume, obs, OrderSide::SELL);
}

void Strategy::sendGrid(int start_level, const double& amount, const DataRow& obs, OrderSide side) {
	std::string sideStr = side == OrderSide::BUY ? "bids[" : "asks[";
	auto refPrice = side == OrderSide::SELL ? obs.getBestAskPrice() : obs.getBestBidPrice();
	auto trade_amount = instrument.getTradeAmount(amount, refPrice);

	for (int ii = 0; ii < 5; ++ii) {
		if (trade_amount >= instrument.getMinAmount()) {
			auto level = ii + start_level;
			std::string key = sideStr + std::to_string(level) + "].price";
			if (obs.data.find(key) != obs.data.end()) {
				double price = obs.data.at(key);
				this->exchange.quote(++order_id, side, price, trade_amount);
			}
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