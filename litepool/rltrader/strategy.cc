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
	               int maxTicks)
	:instrument(instr), exchange(exch),
	 position(instr, balance, pos_amount, avg_price),
	 order_id(0), max_ticks(maxTicks) {

	assert(max_ticks >= 5);
}

void Strategy::reset(const double& position_amount, const double& avg_price) {
	this->exchange.reset();
	this->position.reset(position_amount, avg_price);
	this->order_id = 0;
}

void Strategy::quote(int buy_spread, int sell_spread, int buy_percent, int sell_percent) {
	const auto& obs = this->exchange.getObs();
	exchange.cancelBuys();
	exchange.cancelSells();
	auto posInfo = position.getPositionInfo(obs.getBestBidPrice(), obs.getBestAskPrice());
	auto leverage = posInfo.leverage;
        auto inventoryPnL = posInfo.inventoryPnL;
        auto initBalance = position.getInitialBalance();

	double buy_volume = initBalance * buy_percent / 500.0;
	double sell_volume = initBalance * sell_percent / 500.0;
        int buy_levels = 5;
        int sell_levels = 5;        

        if (inventoryPnL >= 0.0025 * initBalance) {
            buy_spread = leverage < 0 ? 0 : buy_spread;
            buy_volume = leverage < 0 ? std::abs(initBalance * leverage) : buy_volume;
            buy_levels = leverage < 0 ? 1 : buy_levels;
            sell_spread = leverage > 0 ? 0 : sell_spread;
            sell_volume = leverage > 0 ? std::abs(initBalance * leverage) : sell_volume;
            sell_levels = leverage > 0 ? 1 : sell_levels;
        } else {
            if (leverage > 0.15) {
                buy_spread += 2;
                buy_spread *= 1 + static_cast<int>(4 * leverage);
            } else if (leverage < -0.15) {
                sell_spread += 2;
                sell_spread *= 1 + static_cast<int>(-4 * leverage);
            }
        }

	this->sendGrid(buy_levels, buy_spread, buy_volume, obs, OrderSide::BUY);
	this->sendGrid(sell_levels, sell_spread, sell_volume, obs, OrderSide::SELL);
}

void Strategy::sendGrid(int levels, int start_level, const double& amount, const DataRow& obs, OrderSide side) {
	std::string sideStr = side == OrderSide::BUY ? "bids[" : "asks[";
	auto refPrice = side == OrderSide::SELL ? obs.getBestAskPrice() : obs.getBestBidPrice();
	auto trade_amount = instrument.getTradeAmount(amount, refPrice);

        for (int ii = 0; ii < levels; ++ii) {
             if (trade_amount >= instrument.getMinAmount()) {
	          std::string key = sideStr + std::to_string(start_level + ii) + "].price";
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
