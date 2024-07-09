#pragma once

#include <memory>
#include "exchange.h"
#include "position.h"
#include "signal_builder.h"

namespace Simulator {
	class Strategy {
	public:
		Strategy(BaseInstrument& instr, Exchange& exch, const double& balance,
			const double& pos_amount, const double& avg_price,
			const double& areaFactor, int maxTicks);
			
		void reset(int time_index, const double& position_amount, const double& avg_price);

		bool quote(int bid_tick_spread, int ask_tick_spread,
			const double& buyVolumeAngle, const double& sellVolumeAngle);

		void fetchInfo(PositionInfo& info, const double& bidPrice, const double& askPrice);
                
                long numOfTrades() const { return position.getNumberOfTrades(); }	
	private:
		BaseInstrument& instrument;
		Exchange& exchange;
		Position position;
		int order_id;
		double maxFactor;
		int max_ticks;
		std::unique_ptr<SignalBuilder> signal_ptr;
		void sendGrid(const double& angle, const DataRow& obs, OrderSide side);
	};
}
