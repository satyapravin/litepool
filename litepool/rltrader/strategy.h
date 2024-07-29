#pragma once

#include <memory>
#include "exchange.h"
#include "position.h"

namespace Simulator {
	class Strategy {
	public:
		Strategy(BaseInstrument& instr, Exchange& exch, const double& balance,
			const double& pos_amount, const double& avg_price,
			const double& areaFactor, int maxTicks);
			
		void reset(int time_index, const double& position_amount, const double& avg_price);

		void quote(const double& buyPercent, const double& sellPercent,
			       const double& buyRatio, const double& sellRatio,
			       const double& spread, const double& skew);

		Position& getPosition() { return position; }

		bool next();

	private:
		BaseInstrument& instrument;
		Exchange& exchange;
		Position position;
		int order_id;
		double maxFactor;
		int max_ticks;
		void sendGrid(const double& percentCapital,
		              const double& spreadRatio,
		              const double& spread,
		              const double& skew,
		              const DataRow& obs,
		              OrderSide side);
	};
}
