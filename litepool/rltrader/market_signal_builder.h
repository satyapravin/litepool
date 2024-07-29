#pragma once

#include <vector>
#include <string>
#include <memory>
#include "orderbook.h"
#include "circ_buffer.h"
#include "circ_table.h"

namespace Simulator {
    struct volume_signal_repository {
        double volume_imbalance_signal_0 = 0;
        double volume_imbalance_signal_1 = 0;
        double volume_imbalance_signal_2 = 0;
        double volume_imbalance_signal_3 = 0;
        double volume_imbalance_signal_4 = 0;
    } __attribute((packed));

    struct spread_signal_repository {
        double norm_vwap_bid_spread_signal_0 = 0;
        double norm_vwap_bid_spread_signal_1 = 0;
        double norm_vwap_bid_spread_signal_2 = 0;
        double norm_vwap_bid_spread_signal_3 = 0;
        double norm_vwap_bid_spread_signal_4 = 0;

        double norm_vwap_ask_spread_signal_0 = 0;
        double norm_vwap_ask_spread_signal_1 = 0;
        double norm_vwap_ask_spread_signal_2 = 0;
        double norm_vwap_ask_spread_signal_3 = 0;
        double norm_vwap_ask_spread_signal_4 = 0;

        double micro_spread_signal_0 = 0;
        double micro_spread_signal_1 = 0;
        double micro_spread_signal_2 = 0;
        double micro_spread_signal_3 = 0;
        double micro_spread_signal_4 = 0;

        double bid_fill_spread_signal_0 = 0;
        double bid_fill_spread_signal_1 = 0;
        double bid_fill_spread_signal_2 = 0;
        double bid_fill_spread_signal_3 = 0;
        double bid_fill_spread_signal_4 = 0;

        double ask_fill_spread_signal_0 = 0;
        double ask_fill_spread_signal_1 = 0;
        double ask_fill_spread_signal_2 = 0;
        double ask_fill_spread_signal_3 = 0;
        double ask_fill_spread_signal_4 = 0;
    } __attribute((packed));

    struct price_signal_repository {
        double mid_price_signal = 0;
        double vwap_bid_price_signal_0 = 0;
        double vwap_bid_price_signal_1 = 0;
        double vwap_bid_price_signal_2 = 0;
        double vwap_bid_price_signal_3 = 0;
        double vwap_bid_price_signal_4 = 0;

        double vwap_ask_price_signal_0 = 0;
        double vwap_ask_price_signal_1 = 0;
        double vwap_ask_price_signal_2 = 0;
        double vwap_ask_price_signal_3 = 0;
        double vwap_ask_price_signal_4 = 0;

        double norm_vwap_bid_price_signal_0 = 0;
        double norm_vwap_bid_price_signal_1 = 0;
        double norm_vwap_bid_price_signal_2 = 0;
        double norm_vwap_bid_price_signal_3 = 0;
        double norm_vwap_bid_price_signal_4 = 0;

        double norm_vwap_ask_price_signal_0 = 0;
        double norm_vwap_ask_price_signal_1 = 0;
        double norm_vwap_ask_price_signal_2 = 0;
        double norm_vwap_ask_price_signal_3 = 0;
        double norm_vwap_ask_price_signal_4 = 0;

        double micro_price_signal_0 = 0;
        double micro_price_signal_1 = 0;
        double micro_price_signal_2 = 0;
        double micro_price_signal_3 = 0;
        double micro_price_signal_4 = 0;

        double bid_fill_price_signal_0 = 0;
        double bid_fill_price_signal_1 = 0;
        double bid_fill_price_signal_2 = 0;
        double bid_fill_price_signal_3 = 0;
        double bid_fill_price_signal_4 = 0;

        double ask_fill_price_signal_0 = 0;
        double ask_fill_price_signal_1 = 0;
        double ask_fill_price_signal_2 = 0;
        double ask_fill_price_signal_3 = 0;
        double ask_fill_price_signal_4 = 0;
    } __attribute((packed));

class MarketSignalBuilder {
public:
    MarketSignalBuilder(u_int bookhistory, u_int price_history, u_int depth);

    std::vector<double> add_book(Orderbook& lob);

    price_signal_repository& get_price_signals(int lag);

    [[nodiscard]] spread_signal_repository& get_spread_signals() const;

    [[nodiscard]] volume_signal_repository& get_volume_signals() const;

private:
    void compute_signals(Orderbook& book);

    static void compute_price_signals( price_signal_repository& repo,
                                      const std::vector<double>& current_bid_prices,
                                      const std::vector<double>& current_ask_prices,
                                      const std::vector<double>& current_bid_sizes,
                                      const std::vector<double>& current_ask_sizes,
                                      const std::vector<double>& cum_bid_sizes,
                                      const std::vector<double>& cum_ask_sizes,
                                      const std::vector<double>& cum_bid_amounts,
                                      const std::vector<double>& cum_ask_amounts);

    void compute_spread_signals(const price_signal_repository& repo) const;

    void compute_volume_signals(const std::vector<double>& bid_sizes,
                                const std::vector<double>& ask_sizes,
                                const std::vector<double>& cum_bid_sizes,
                                const std::vector<double>& cum_ask_sizes) const;

private:
    std::unique_ptr<spread_signal_repository> raw_spread_signals;
    std::unique_ptr<volume_signal_repository> raw_volume_signals;
};
}
