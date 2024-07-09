#pragma once

#include <deque>
#include <vector>
#include <map>
#include <unordered_map>
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

class SignalBuilder {
public:
    enum signal_type { raw=0, mean=1, ssr=2, norm=3 };

    SignalBuilder(u_int bookhistory=10, u_int price_history=10);

    bool is_data_ready() const;

    std::vector<double> add_book(Orderbook& lob);

    long get_tick_count() { return processed_counter; }

    price_signal_repository& get_price_signals(signal_type sigtype);

    spread_signal_repository& get_spread_signals(signal_type sigtype);

    volume_signal_repository& get_volume_signals(signal_type sigtype);

    price_signal_repository& get_velocity_1_signals(signal_type sigtype);

    price_signal_repository& get_velocity_10_signals(signal_type sigtype);

    price_signal_repository& get_volatility_1_signals(signal_type sigtype);

    price_signal_repository& get_volatility_10_signals(signal_type sigtype);

    double normalize(const double& raw_signal, const double& alpha, double& mean, double& ssr);

private:
    void insert_norm_price_signals(std::vector<double>& signals);

    void insert_norm_spread_signals(std::vector<double>& signals);

    void insert_norm_volume_signals(std::vector<double>& signals);

    void insert_norm_velocity_1_signals(std::vector<double>& signals);

    void insert_norm_velocity_10_signals(std::vector<double>& signals);

    void insert_norm_volatility_1_signals(std::vector<double>& signals);

    void insert_norm_volatility_10_signals(std::vector<double>& signals);

    void compute_signals();

    void compute_price_signals( price_signal_repository& repo,
                                const std::vector<double>& current_bid_prices,
                                const std::vector<double>& current_ask_prices,
                                const std::vector<double>& current_bid_sizes,
                                const std::vector<double>& current_ask_sizes,
                                const std::vector<double>& cum_bid_sizes,
                                const std::vector<double>& cum_ask_sizes,
                                const std::vector<double>& cum_bid_amounts,
                                const std::vector<double>& cum_ask_amounts);

    void compute_spread_signals(price_signal_repository& repo);

    void compute_volume_signals(std::vector<double>& bid_sizes,
                                std::vector<double>& ask_sizes,
                                std::vector<double>& cum_bid_sizes,
                                std::vector<double>& cum_ask_sizes);


    void compute_velocity_signals();

    void compute_volatility_signals();

    void normalize_signals();

    void normalize_price_signals();

    void normalize_spread_signals();

    void normalize_volume_signals();

    void normalize_velocity_1_signals();

    void normalize_velocity_10_signals();

    void normalize_volatility_1_signals();

    void normalize_volatility_10_signals();

    double compute_ofi(int lag);

    double compute_vwap_ofi(int lag);

private:
    u_long processed_counter = 0;
    u_int minimum_required = 30;
    double norm_alpha = 2.0 / 1201;
    TemporalTable bid_prices;
    TemporalTable ask_prices;
    TemporalTable bid_sizes;
    TemporalTable ask_sizes;

    TemporalBuffer<price_signal_repository> raw_price_signals;
    std::unique_ptr<price_signal_repository> raw_price_signal_mean;
    std::unique_ptr<price_signal_repository> raw_price_signal_ssqr;
    std::unique_ptr<price_signal_repository> raw_price_normalized_signals;

    std::unique_ptr<spread_signal_repository> raw_spread_signals;
    std::unique_ptr<spread_signal_repository> raw_spread_signal_mean;
    std::unique_ptr<spread_signal_repository> raw_spread_signal_ssqr;
    std::unique_ptr<spread_signal_repository> raw_spread_normalized_signals;

    std::unique_ptr<volume_signal_repository> raw_volume_signals;
    std::unique_ptr<volume_signal_repository> raw_volume_signal_mean;
    std::unique_ptr<volume_signal_repository> raw_volume_signal_ssqr;
    std::unique_ptr<volume_signal_repository> raw_volume_normalized_signals;

    std::unique_ptr<price_signal_repository> raw_price_signal_velocities_1;
    std::unique_ptr<price_signal_repository>  raw_price_signal_velocity_1_mean;
    std::unique_ptr<price_signal_repository>  raw_price_signal_velocity_1_ssqr;
    std::unique_ptr<price_signal_repository>  raw_price_velocity_1_normalized_signals;

    std::unique_ptr<price_signal_repository> raw_price_signal_velocities_10;
    std::unique_ptr<price_signal_repository>  raw_price_signal_velocity_10_mean;
    std::unique_ptr<price_signal_repository>  raw_price_signal_velocity_10_ssqr;
    std::unique_ptr<price_signal_repository>  raw_price_velocity_10_normalized_signals;

    std::unique_ptr<price_signal_repository> raw_price_vol_1_mean;
    std::unique_ptr<price_signal_repository> raw_price_vol_1_ssqr;
    std::unique_ptr<price_signal_repository> raw_price_vol_1_normalized_signals;

    std::unique_ptr<price_signal_repository> raw_price_vol_10_mean;
    std::unique_ptr<price_signal_repository> raw_price_vol_10_ssqr;
    std::unique_ptr<price_signal_repository> raw_price_vol_10_normalized_signals;
};
}
