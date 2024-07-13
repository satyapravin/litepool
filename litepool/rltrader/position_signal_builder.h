#pragma once

#include <vector>
#include <string>
#include <memory>
#include "position.h"
#include "circ_buffer.h"

namespace Simulator {
    struct position_signal_repository {
        double net_position = 0;
        double relative_price = 0;
        double total_drawdown = 0;
        double realized_pnl_drawdown = 0;
        double inventory_pnl_drawdown = 0;
        double total_pnl = 0;
        double realized_pnl = 0;
        double inventory_pnl = 0;
    };
    
    class PositionSignalBuilder {
    public:
        enum signal_type { raw=0, mean=1, ssr=2, norm=3 };

        PositionSignalBuilder();

        std::vector<double> add_info(PositionInfo& info, double& bid_price, double& ask_price);

        position_signal_repository& get_position_signals(signal_type sigtype);

        position_signal_repository& get_velocity_signals(signal_type sigtype);

        position_signal_repository& get_volatility_signals(signal_type sigtype);

        long long get_step_count() { return processed_counter; }

    private:
        void compute_signals(PositionInfo& info, double& bid_price, double& ask_price);
        void compute_velocity();
        void compute_volatility();
        long long processed_counter = 0;
        double alpha = 2.0 / 9001;
        TemporalBuffer<position_signal_repository> raw_signals;
        std::unique_ptr<position_signal_repository> mean_raw_signals;
        std::unique_ptr<position_signal_repository> ssr_raw_signals;
        std::unique_ptr<position_signal_repository> norm_raw_signals;

        std::unique_ptr<position_signal_repository> velocity_10_signals;
        std::unique_ptr<position_signal_repository> mean_velocity_10_signals;
        std::unique_ptr<position_signal_repository> ssr_velocity_10_signals;
        std::unique_ptr<position_signal_repository> norm_velocity_10_signals;

        std::unique_ptr<position_signal_repository> mean_volatility_10_signals;
        std::unique_ptr<position_signal_repository> ssr_volatility_10_signals;
        std::unique_ptr<position_signal_repository> norm_volatility_10_signals;
    };
}
