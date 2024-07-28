#pragma once
#include <vector>
#include <memory>
#include "position.h"

namespace Simulator {
    struct trade_signal_repository {
        double buy_num_trade_ratio = 0;
        double sell_num_trade_ratio = 0;
        double buy_amount_ratio = 0;
        double sell_amount_ratio = 0;
        double relative_buy_price = 0;
        double relative_sell_price = 0;
    }__attribute((packed));

    class TradeSignalBuilder {
        public:
            TradeSignalBuilder();

            std::vector<double> add_trade(const TradeInfo& info, const double& bid_price, const double& ask_price);

            [[nodiscard]] trade_signal_repository& get_trade_signals() const;


        private:
            void compute_trade_signals(const TradeInfo& info, const double& bid_price, const double& ask_price);
            std::unique_ptr<trade_signal_repository> raw_signals;
    };
}