#include <vector>
#include <string>
#include <memory>
#include "circ_buffer.h"
#include "position.h"
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

            std::vector<double> add_trade(TradeInfo& info, double& bid_price, double& ask_price);

            trade_signal_repository& get_trade_signals(signal_type sigtype);

            trade_signal_repository& get_velocity_signals(signal_type sigtype);

            trade_signal_repository& get_volatility_signals(signal_type sigtype);

        private:
            void compute_trade_signals(TradeInfo& info, double& bid_price, double& ask_price);
            void compute_velocity_signals();
            void compute_volatility_signals();

            long long processed_counter = 0;
            double alpha = 2.0 / 9001;
            TemporalBuffer<trade_signal_repository> raw_signals;
            std::unique_ptr<trade_signal_repository> mean_raw_signals;
            std::unique_ptr<trade_signal_repository> ssr_raw_signals;
            std::unique_ptr<trade_signal_repository> norm_raw_signals;

            std::unique_ptr<trade_signal_repository> velocity_10_signals;
            std::unique_ptr<trade_signal_repository> mean_velocity_10_signals;
            std::unique_ptr<trade_signal_repository> ssr_velocity_10_signals;
            std::unique_ptr<trade_signal_repository> norm_velocity_10_signals;

            std::unique_ptr<trade_signal_repository> mean_volatility_10_signals;
            std::unique_ptr<trade_signal_repository> ssr_volatility_10_signals;
            std::unique_ptr<trade_signal_repository> norm_volatility_10_signals;
    };
}