#pragma once
#include "base_instrument.h"
#include "order.h"

namespace Simulator {
	struct PositionInfo {
        double netPosition = 0;
        double balance = 0;
        double averagePrice = 0;
        double tradingPnL = 0;
        double inventoryPnL = 0;
        double leverage = 0;
	};

    class Position {
    private:
        BaseInstrument& instrument;
        double averagePrice = 0.0;
        double netQuantity = 0.0;
        double totalFee = 0.0;
        int numOfTrades = 0;
        double totalQuantity = 0.0;
        double initialBalance = 0.0;
        double balance = 0.0;


    public:
        Position(BaseInstrument& instr, const double& aBalance, const double& initialQty, const double& initialAvgprice);
        void reset(const double& initialQty, const double& initialAvgprice);
        void fetchInfo(PositionInfo& info, const double& bidPrice, const double& askPrice) const;
        void onFill(const Order& order, bool is_maker);
        double inventoryPnL(const double& price) const;
        double getInitialBalance() const { return initialBalance; }
        long getNumberOfTrades() const { return numOfTrades; }
    };
}
