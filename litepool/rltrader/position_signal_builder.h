#pragma once

#include <circ_buffer.h>
#include <vector>
#include <memory>
#include "position.h"

namespace RLTrader {
    struct position_signal_repository {
        double leverage = 0;
        double net_position = 0;
        double relative_price = 0;
        double total_drawdown = 0;
        double realized_pnl_drawdown = 0;
        double inventory_pnl_drawdown = 0;
        double total_pnl = 0;
        double realized_pnl = 0;
        double inventory_pnl = 0;
    } __attribute((packed));
    
    class PositionSignalBuilder {
    public:
        PositionSignalBuilder();

        std::vector<double> add_info(const PositionInfo& info, const double& bid_price, const double& ask_price);


    private:
        void compute_signals(const PositionInfo& info, const double& bid_price, const double& ask_price);

        double max_inventory_pnl = 0;
        double max_trading_pnl = 0;
        std::unique_ptr<position_signal_repository> raw_previous_signals;
        std::unique_ptr<position_signal_repository> raw_spread_signals;
    };
}
