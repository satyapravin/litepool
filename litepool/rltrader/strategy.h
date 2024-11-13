#pragma once

#include <memory>
#include "exchange.h"
#include "position.h"

namespace Simulator {
	class Strategy {
	public:
		Strategy(BaseInstrument& instr, Exchange& exch, const double& balance,
			const double& pos_amount, const double& avg_price,
			int maxTicks);
			
		void reset(const double& position_amount, const double& avg_price);

		void quote(int buy_spread, int sell_spread, int buy_percent, int sell_percent, int buy_level, int sell_level);

		Position& getPosition() { return position; }

		void next();

	private:
		BaseInstrument& instrument;
		Exchange& exchange;
		Position position;
		int order_id;
		int max_ticks;
		void sendGrid(int levels, int start_level,
			      const double& amount, const DataRow& obs,
		              OrderSide side);
	};
}
