#include "strategy.h"
#include <string>
#include <cmath>
#include <cassert>
#include <algorithm>
#include "orderbook.h"

using namespace Simulator;

Strategy::Strategy(BaseInstrument& instr, Exchange& exch, const double& balance,
	               const double& pos_amount, const double& avg_price,
	               const double& areaFactor, int maxTicks)
	:instrument(instr), exchange(exch),
	 position(instr, balance, pos_amount, avg_price),
	 order_id(0), maxFactor(areaFactor), max_ticks(maxTicks) {

	assert(max_ticks >= 5);
	signal_ptr = std::make_unique<SignalBuilder>(20, 20);
}

void Strategy::reset(int time_index, const double& position_amount, const double& avg_price) {
	this->exchange.reset(time_index);
	this->position.reset(position_amount, avg_price);
	this->order_id = 0;
	signal_ptr.reset(std::make_unique<SignalBuilder>(20, 20).release());
}

bool Strategy::quote(int bid_tick_spread, int ask_tick_spread, 
	                 const double& buyVolumeAngle, const double& sellVolumeAngle) {
	assert((buyVolumeAngle >= 2 && buyVolumeAngle <= 88));
	assert((sellVolumeAngle >= 2 && sellVolumeAngle <= 88));
	const auto& obs = this->exchange.getObs();
	bid_tick_spread = std::max(bid_tick_spread, 10);
	ask_tick_spread = std::max(ask_tick_spread, 10);
	this->sendGrid(buyVolumeAngle, obs, OrderSide::BUY);
	this->sendGrid(sellVolumeAngle, obs, OrderSide::SELL);
	return this->exchange.next();
}

void Strategy::sendGrid(const double& angle, const DataRow& obs, OrderSide side) {
	static const double R2D = 3.14159265358979323846 / 180.0;
	double tanAngle = std::tan(angle * R2D);
	double base = std::sqrt(maxFactor * tanAngle);
	int height = static_cast<int>(maxFactor / base);
	double area = 0;
	double min_volume = this->instrument.getMinAmount();
	double ref_price = side == OrderSide::BUY ? obs.data.at("bids[0].price") : obs.data.at("asks[0].price");
	double initial_balance = this->position.getInitialBalance() * ref_price / (maxFactor / 2.0);
	double totalVolume = 0;
	std::string sideStr = side == OrderSide::BUY ? "bids[" : "asks[";

	for (int ii = 0; ii < height; ++ii) {
		if (ii == 0) {
			area += 0.5 * (ii + 1) * tanAngle * (ii + 1) * initial_balance;
		}
		else {
			area += 0.5 * (tanAngle * (2 * ii + 1)) * initial_balance;
		}

		totalVolume += area;
		if (area > min_volume) {
			double volume = std::round(area / min_volume) * min_volume;
			if (ii < max_ticks) {
				double price = obs.data.at(sideStr + std::to_string(ii) + "].price");
				this->exchange.quote(++order_id, side, price, volume);
			}
			else {
				double prevPrice = obs.data.at(sideStr + std::to_string(max_ticks - 2) + "].price");
				double currPrice = obs.data.at(sideStr + std::to_string(max_ticks - 1) + "].price");
				double price = currPrice + (currPrice - prevPrice) * ii * instrument.getTickSize();
				this->exchange.quote(++order_id, side, price, volume);
			}

			area -= volume;
		}
	}
}

void Strategy::fetchInfo(PositionInfo& info, const double& bidPrice, const double& askPrice) {
	return position.fetchInfo(info, bidPrice, askPrice);
}
