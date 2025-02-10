#include "strategy.h"
#include <string>
#include <cmath>
#include <cassert>
#include "orderbook.h"
#include "position_signal_builder.h"
#include "trade_signal_builder.h"

using namespace RLTrader;

Strategy::Strategy(BaseInstrument& instr, BaseExchange& exch, const double& balance, int maxTicks)
	:instrument(instr), exchange(exch),
	 position(instr, balance, 0, 0),
	 order_id(0), max_ticks(maxTicks) {

	assert(max_ticks >= 5);
}

void Strategy::reset() {
	this->exchange.reset();
	double initQty = 0;
	double avgPrice = 0;
	this->exchange.fetchPosition(initQty, avgPrice);
	this->position.reset(initQty, avgPrice);
	this->order_id = 0;
}

void Strategy::quote(int buy_spread, int sell_spread, int buy_percent, int sell_percent) {
	auto book = this->exchange.getBook();
	exchange.cancelOrders();
	auto posInfo = position.getPositionInfo(book.bid_prices[0], book.ask_prices[0]);
	auto leverage = posInfo.leverage;
        auto inventoryPnL = posInfo.inventoryPnL;
        auto initBalance = position.getInitialBalance();
        double buy_denom = 100;
        double sell_denom = 100;

	double buy_volume = initBalance * buy_percent / buy_denom;
	double sell_volume = initBalance * sell_percent / sell_denom;
        int skew = static_cast<int>(2 * leverage);
        buy_spread = std::max(0, buy_spread + skew);
        sell_spread = std::max(0, sell_spread - skew);

        if (buy_volume > 0 && buy_spread >= 0) this->sendGrid(1, buy_spread, buy_volume, book, OrderSide::BUY);
        if (sell_volume > 0 && sell_spread >= 0) this->sendGrid(1, sell_spread, sell_volume, book, OrderSide::SELL);
}

void Strategy::sendGrid(int levels, int start_level, const double& amount, const Orderbook& book, OrderSide side) {
	auto refPrices = side == OrderSide::SELL ? book.ask_prices : book.bid_prices;

        for (int ii = 0; ii < levels; ++ii) {
	     auto trade_amount = instrument.getTradeAmount(amount, refPrices[0]);
             if (trade_amount >= instrument.getMinAmount()) {
             	this->exchange.quote(++order_id, side, refPrices[ii + start_level], trade_amount);
             }
        }
}

void Strategy::next() {
    auto fills = exchange.getFills();
    for(auto order: fills) {
        position.onFill(order, true);
    }
}
