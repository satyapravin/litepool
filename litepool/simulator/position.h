#pragma once
#include "base_instrument.h"
#include "order.h"

namespace Simulator {
	struct PositionInfo {
        double balance = 0;
        long tradeCount = 0;
        double averagePrice = 0;
        double tradingPnL = 0;
        double inventoryPnL = 0;
        double leverage = 0;
	};

    class Position {
    private:
        double averagePrice = 0.0;
        double totalQuantity = 0.0;
        double totalFee = 0.0;
        int numOfTrades = 0;
        double totalNotional = 0.0;
        double initialBalance = 0.0;
        double balance = 0.0;
        BaseInstrument& instrument;

    public:
        Position(BaseInstrument& instr, const double& balance, 
            const double& initialQty, const double& initialAvgprice);
        void reset(const double& initialQty, const double& initialAvgprice);
        void fetchInfo(PositionInfo& info, const double& bidPrice, const double& askPrice) const;
        void onFill(const Order& order, bool is_maker);
        double inventoryPnL(const double& price) const;
        double getInitialBalance() const { return initialBalance; }
    };
}