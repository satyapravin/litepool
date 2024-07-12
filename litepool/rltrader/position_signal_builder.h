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
        double total_trade_amount = 0;
        double pnl = 0;
        double drawdown = 0;
        double realized_pnl = 0;
        double inventory_pnl = 0;
    };

    
    class PositionSignalBuilder {
    public:
        enum signal_type { raw=0, mean=1, ssr=2, norm=3 };

        PositionSignalBuilder();

        std::vector<double> add_info(PositionInfo& info);

        position_signal_repository& get_position_signals(signal_type sigtype);

        position_signal_repository& get_velocity_signals(signal_type sigtype);

        position_signal_repository& get_volatility_signals(signal_type sigtype);

        double get_steps_until_episode() { return steps_until_episode; }

        double get_steps_since_episode() { return steps_since_episode; }

        long long get_step_count() { return processed_counter; }

        bool is_data_ready() const { return processed_counter > minimum_required; };

    private:
        long long processed_counter = 0;
        double steps_until_episode = 0;
        double steps_since_episode = 0;
        uint16_t minimum_required = 10;
        double alpha = 2.0 / 9001;
        TemporalBuffer<position_signal_repository> raw_signals;
        std::unique_ptr<position_signal_repository> mean_raw_signals;
        std::unique_ptr<position_signal_repository> ssr_raw_signals;
        std::unique_ptr<position_signal_repository> norm_raw_signals;

        std::unique_ptr<position_signal_repository> velocity_10_signals;
        std::unique_ptr<position_signal_repository> mean_velocity_10_signals;
        std::unique_ptr<position_signal_repository> ssr_velocity_10_signals;
        std::unique_ptr<position_signal_repository> norm_velocity_10_signals;

        std::unique_ptr<position_signal_repository> volatility_10_signals;
        std::unique_ptr<position_signal_repository> mean_volatility_10_signals;
        std::unique_ptr<position_signal_repository> ssr_volatility_10_signals;
        std::unique_ptr<position_signal_repository> norm_volatility_10_signals;
    };
}
