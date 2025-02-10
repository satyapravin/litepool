#pragma once

#include <memory>

#include "base_exchange.h"
#include "orderbook.h"
#include "position.h"

namespace RLTrader {
	class Strategy {
	public:
		Strategy(BaseInstrument& instr, BaseExchange& exch, const double& balance, int maxTicks);
			
		void reset();

		void quote(int buy_spread, int sell_spread, int buy_percent, int sell_percent);

		Position& getPosition() { return position; }

		void next();

	private:
		BaseInstrument& instrument;
		BaseExchange& exchange;
		Position position;
		int order_id;
		int max_ticks;
		void sendGrid(int levels, int start_level,
			      const double& amount, const Orderbook& book,
		              OrderSide side);
	};
}
