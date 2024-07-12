#include <vector>
#include <string>
#include <memory>
#include "circ_buffer.h"
#include "order.h"

namespace Simulator {
    struct trade_signal_repository {
        double buy_num_trade_ratio = 0;
        double sell_num_trade_ratio = 0;
        double number_of_trades = 0;
        double average_trade_amount = 0;
        double buy_amount_ratio = 0;
        double sell_amount_ratio = 0;
        double relative_buy_price = 0;
        double relative_sell_price = 0;
    };

    class TradeSignalBuilder {
        public:
            enum signal_type { raw=0, mean=1, ssr=2, norm=3 };

            TradeSignalBuilder();

            std::vector<double> add_trade(double price, double amount, OrderSide side);

            trade_signal_repository& get_trade_signals(signal_type sigtype);

            trade_signal_repository& get_velocity_signals(signal_type sigtype);

            trade_signal_repository& get_volatility_signals(signal_type sigtype);

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
            TemporalBuffer<trade_signal_repository> raw_signals;
            std::unique_ptr<trade_signal_repository> mean_raw_signals;
            std::unique_ptr<trade_signal_repository> ssr_raw_signals;
            std::unique_ptr<trade_signal_repository> norm_raw_signals;

            std::unique_ptr<trade_signal_repository> velocity_10_signals;
            std::unique_ptr<trade_signal_repository> mean_velocity_10_signals;
            std::unique_ptr<trade_signal_repository> ssr_velocity_10_signals;
            std::unique_ptr<trade_signal_repository> norm_velocity_10_signals;

            std::unique_ptr<trade_signal_repository> volatility_10_signals;
            std::unique_ptr<trade_signal_repository> mean_volatility_10_signals;
            std::unique_ptr<trade_signal_repository> ssr_volatility_10_signals;
            std::unique_ptr<trade_signal_repository> norm_volatility_10_signals;
    };
}