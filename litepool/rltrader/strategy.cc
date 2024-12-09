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
        double buy_denom = 100;
        double sell_denom = 100;

	double buy_volume = initBalance * buy_percent / buy_denom;
	double sell_volume = initBalance * sell_percent / sell_denom;
        int skew = static_cast<int>(3 * leverage);
        buy_spread = std::max(0, buy_spread + skew);
        sell_spread = std::max(0, sell_spread - skew);

        if (buy_volume > 0) this->sendGrid(1, buy_spread, buy_volume, obs, OrderSide::BUY);
        if (sell_volume > 0) this->sendGrid(1, sell_spread, sell_volume, obs, OrderSide::SELL);
}

void Strategy::sendGrid(int levels, int start_level, const double& amount, const DataRow& obs, OrderSide side) {
	std::string sideStr = side == OrderSide::BUY ? "bids[" : "asks[";
	auto refPrice = side == OrderSide::SELL ? obs.getBestAskPrice() : obs.getBestBidPrice();

        for (int ii = 0; ii < levels; ++ii) {
	     auto trade_amount = instrument.getTradeAmount(amount * (1 + ii), refPrice);
             if (trade_amount >= instrument.getMinAmount()) {
	          std::string key = sideStr + std::to_string(start_level + ii * 2) + "].price";
	          if (obs.data.find(key) != obs.data.end()) {
	                double price = obs.data.at(key);
	                this->exchange.quote(++order_id, side, price, trade_amount);
                  } else {
                        return;
                  }
             }
        }
}

void Strategy::next() {
    auto fills = exchange.getFills();
    for(auto order: fills) {
        position.onFill(order, true);
    }
}
