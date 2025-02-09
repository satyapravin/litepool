#pragma once

#include <memory>
#include "sim_exchange.h"
#include "position.h"

namespace Simulator {
	class Strategy {
	public:
		Strategy(BaseInstrument& instr, SimExchange& exch, const double& balance,
			const double& pos_amount, const double& avg_price,
			int maxTicks);
			
		void reset(const double& position_amount, const double& avg_price);

		void quote(int buy_spread, int sell_spread, int buy_percent, int sell_percent);

		Position& getPosition() { return position; }

		void next();

	private:
		BaseInstrument& instrument;
		SimExchange& exchange;
		Position position;
		int order_id;
		int max_ticks;
		void sendGrid(int levels, int start_level,
			      const double& amount, const Orderbook& book,
		              OrderSide side);
	};
}
