#include "market_signal_builder.h"
#include <numeric>
#include <cmath>
#include "rl_macros.h"

using namespace Simulator;

MarketSignalBuilder::MarketSignalBuilder(u_int bookhistory, u_int price_history, u_int depth)
              :bid_prices(bookhistory, depth),
               ask_prices(bookhistory, depth),
               bid_sizes(bookhistory, depth),
               ask_sizes(bookhistory, depth),
               raw_price_signals(price_history),                                                         // price
               raw_spread_signals(std::make_unique<spread_signal_repository>()),                         // spread
               raw_volume_signals(std::make_unique<volume_signal_repository>())                          // volume
{
}


price_signal_repository& MarketSignalBuilder::get_price_signals(int lag) {
    return raw_price_signals.get(lag);
}

spread_signal_repository& MarketSignalBuilder::get_spread_signals() const {
    return *raw_spread_signals;
}

volume_signal_repository& MarketSignalBuilder::get_volume_signals() const {
    return *raw_volume_signals;
}

std::vector<double> cumulative_prod_sum(const std::vector<double>& first, const std::vector<double>& second) {
    std::vector<double> result;
    double cum_product_sum = 0;

    for(auto ii=0; ii < first.size(); ++ii) {
        cum_product_sum += first[ii] * second[ii];
        result.push_back(cum_product_sum);
    }

    return result;
}

std::vector<double> get_cumulative_sizes(const std::vector<double>& sizes) {
    std::vector<double> retval(sizes.size());
    std::partial_sum(sizes.begin(), sizes.end(), retval.begin());
    return retval;
}

double fill_price(const std::vector<double>& bid_prices, const std::vector<double>& bid_sizes, double size) {
    double fill_amount = 0;
    double fill_size = size;
    for(auto ii = 0; ii < bid_prices.size(); ++ii) {
        if (fill_size <= 0) break;
        fill_amount += std::min(bid_sizes[ii], fill_size) * bid_prices[ii];
        fill_size -= bid_sizes[ii];
    }

    if (fill_size > 0) {
        size -= fill_size;
    }

    return fill_amount / size;
}

double micro_price(const double& bid_price, const double& ask_price,
                   const double& bid_size, const double& ask_size) {
    return (bid_price * ask_size + ask_price * bid_size) / (bid_size + ask_size);
}

std::vector<double> MarketSignalBuilder::add_book(Orderbook& book) {
    bid_prices.addRow(book.bid_prices);
    bid_sizes.addRow(book.bid_sizes);
    ask_prices.addRow(book.ask_prices);
    ask_sizes.addRow(book.ask_sizes);

    compute_signals();

    std::vector<double> retval;
    insert_signals(retval, *raw_spread_signals);
    insert_signals(retval, *raw_volume_signals);
    return retval;
}

void MarketSignalBuilder::compute_signals() {
    auto current_bid_prices = bid_prices.get(0);
    auto current_ask_prices = ask_prices.get(0);
    auto current_bid_sizes = bid_sizes.get(0);
    auto current_ask_sizes = ask_sizes.get(0);

    std::vector<double> cum_bid_sizes = get_cumulative_sizes(current_bid_sizes);
    std::vector<double> cum_ask_sizes = get_cumulative_sizes(current_ask_sizes);

    std::vector<double> cum_bid_amounts = cumulative_prod_sum(current_bid_prices, current_bid_sizes);
    std::vector<double> cum_ask_amounts = cumulative_prod_sum(current_ask_prices, current_ask_sizes);

    price_signal_repository repo;

    compute_price_signals(repo,
                          current_bid_prices,
                          current_ask_prices,
                          current_bid_sizes,
                          current_ask_sizes,
                          cum_bid_sizes,
                          cum_ask_sizes,
                          cum_bid_amounts,
                          cum_ask_amounts);

    raw_price_signals.add(repo);

    compute_spread_signals(repo);

    compute_volume_signals(current_bid_sizes, current_ask_sizes,
                           cum_bid_sizes, cum_ask_sizes);
}

void MarketSignalBuilder::compute_volume_signals(const std::vector<double>& bid_sizes,
                                                 const std::vector<double>& ask_sizes,
                                                 const std::vector<double>& cum_bid_sizes,
                                                 const std::vector<double>& cum_ask_sizes) const {
    raw_volume_signals->volume_imbalance_signal_0 = (bid_sizes[0] - ask_sizes[0]) / (bid_sizes[0] + ask_sizes[0]);
    raw_volume_signals->volume_imbalance_signal_1 = (cum_bid_sizes[1] - cum_ask_sizes[1]) / (cum_bid_sizes[1] + cum_ask_sizes[1]);
    raw_volume_signals->volume_imbalance_signal_2 = (cum_bid_sizes[2] - cum_ask_sizes[2]) / (cum_bid_sizes[2] + cum_ask_sizes[2]);
    raw_volume_signals->volume_imbalance_signal_3 = (cum_bid_sizes[3] - cum_ask_sizes[3]) / (cum_bid_sizes[3] + cum_ask_sizes[3]);
    raw_volume_signals->volume_imbalance_signal_4 = (cum_bid_sizes[4] - cum_ask_sizes[4]) / (cum_bid_sizes[4] + cum_ask_sizes[4]);
}

void MarketSignalBuilder::compute_spread_signals(const price_signal_repository& repo) const {
    raw_spread_signals->norm_vwap_bid_spread_signal_0 = (repo.norm_vwap_bid_price_signal_0 - repo.mid_price_signal) * 100.0 / repo.mid_price_signal;
    raw_spread_signals->norm_vwap_bid_spread_signal_1 = (repo.norm_vwap_bid_price_signal_1 - repo.mid_price_signal) * 100.0 / repo.mid_price_signal;
    raw_spread_signals->norm_vwap_bid_spread_signal_2 = (repo.norm_vwap_bid_price_signal_2 - repo.mid_price_signal) * 100.0 / repo.mid_price_signal;
    raw_spread_signals->norm_vwap_bid_spread_signal_3 = (repo.norm_vwap_bid_price_signal_3 - repo.mid_price_signal) * 100.0 / repo.mid_price_signal;
    raw_spread_signals->norm_vwap_bid_spread_signal_4 = (repo.norm_vwap_bid_price_signal_4 - repo.mid_price_signal) * 100.0 / repo.mid_price_signal;

    raw_spread_signals->norm_vwap_ask_spread_signal_0 = (repo.norm_vwap_ask_price_signal_0 - repo.mid_price_signal) * 100.0 / repo.mid_price_signal;
    raw_spread_signals->norm_vwap_ask_spread_signal_1 = (repo.norm_vwap_ask_price_signal_1 - repo.mid_price_signal) * 100.0 / repo.mid_price_signal;
    raw_spread_signals->norm_vwap_ask_spread_signal_2 = (repo.norm_vwap_ask_price_signal_2 - repo.mid_price_signal) * 100.0 / repo.mid_price_signal;
    raw_spread_signals->norm_vwap_ask_spread_signal_3 = (repo.norm_vwap_ask_price_signal_3 - repo.mid_price_signal) * 100.0 / repo.mid_price_signal;
    raw_spread_signals->norm_vwap_ask_spread_signal_4 = (repo.norm_vwap_ask_price_signal_4 - repo.mid_price_signal) * 100.0 / repo.mid_price_signal;

    raw_spread_signals->micro_spread_signal_0 = (repo.micro_price_signal_0 - repo.mid_price_signal) * 100.0 / repo.mid_price_signal;
    raw_spread_signals->micro_spread_signal_1 = (repo.micro_price_signal_1 - repo.mid_price_signal) * 100.0 / repo.mid_price_signal;
    raw_spread_signals->micro_spread_signal_2 = (repo.micro_price_signal_2 - repo.mid_price_signal) * 100.0 / repo.mid_price_signal;
    raw_spread_signals->micro_spread_signal_3 = (repo.micro_price_signal_3 - repo.mid_price_signal) * 100.0 / repo.mid_price_signal;
    raw_spread_signals->micro_spread_signal_4 = (repo.micro_price_signal_4 - repo.mid_price_signal) * 100.0 / repo.mid_price_signal;

    raw_spread_signals->bid_fill_spread_signal_0 = (repo.bid_fill_price_signal_0 - repo.mid_price_signal) * 100.0 / repo.mid_price_signal;
    raw_spread_signals->bid_fill_spread_signal_1 = (repo.bid_fill_price_signal_1 - repo.mid_price_signal) * 100.0 / repo.mid_price_signal;
    raw_spread_signals->bid_fill_spread_signal_2 = (repo.bid_fill_price_signal_2 - repo.mid_price_signal) * 100.0 / repo.mid_price_signal;
    raw_spread_signals->bid_fill_spread_signal_3 = (repo.bid_fill_price_signal_3 - repo.mid_price_signal) * 100.0 / repo.mid_price_signal;
    raw_spread_signals->bid_fill_spread_signal_4 = (repo.bid_fill_price_signal_4 - repo.mid_price_signal) * 100.0 / repo.mid_price_signal;

    raw_spread_signals->ask_fill_spread_signal_0 = (repo.ask_fill_price_signal_0 - repo.mid_price_signal) * 100.0 / repo.mid_price_signal;
    raw_spread_signals->ask_fill_spread_signal_1 = (repo.ask_fill_price_signal_1 - repo.mid_price_signal) * 100.0 / repo.mid_price_signal;
    raw_spread_signals->ask_fill_spread_signal_2 = (repo.ask_fill_price_signal_2 - repo.mid_price_signal) * 100.0 / repo.mid_price_signal;
    raw_spread_signals->ask_fill_spread_signal_3 = (repo.ask_fill_price_signal_3 - repo.mid_price_signal) * 100.0 / repo.mid_price_signal;
    raw_spread_signals->ask_fill_spread_signal_4 = (repo.ask_fill_price_signal_4 - repo.mid_price_signal) * 100.0 / repo.mid_price_signal;
}

void MarketSignalBuilder::compute_price_signals(price_signal_repository& raw_price_repo,
                                          const std::vector<double>& current_bid_prices,
                                          const std::vector<double>& current_ask_prices,
                                          const std::vector<double>& current_bid_sizes,
                                          const std::vector<double>& current_ask_sizes,
                                          const std::vector<double>& cum_bid_sizes,
                                          const std::vector<double>& cum_ask_sizes,
                                          const std::vector<double>& cum_bid_amounts,
                                          const std::vector<double>& cum_ask_amounts) {
    raw_price_repo.mid_price_signal = (current_bid_prices[0] + current_ask_prices[0]) * 0.5;
    raw_price_repo.vwap_bid_price_signal_0 = cum_bid_amounts[0] / cum_bid_sizes[0];
    raw_price_repo.vwap_bid_price_signal_1 = cum_bid_amounts[1] / cum_bid_sizes[1];
    raw_price_repo.vwap_bid_price_signal_2 = cum_bid_amounts[2] / cum_bid_sizes[2];
    raw_price_repo.vwap_bid_price_signal_3 = cum_bid_amounts[3] / cum_bid_sizes[3];
    raw_price_repo.vwap_bid_price_signal_4 = cum_bid_amounts[4] / cum_bid_sizes[4];

    raw_price_repo.vwap_ask_price_signal_0 = cum_ask_amounts[0] / cum_ask_sizes[0];
    raw_price_repo.vwap_ask_price_signal_1 = cum_ask_amounts[1] / cum_ask_sizes[1];
    raw_price_repo.vwap_ask_price_signal_2 = cum_ask_amounts[2] / cum_ask_sizes[2];
    raw_price_repo.vwap_ask_price_signal_3 = cum_ask_amounts[3] / cum_ask_sizes[3];
    raw_price_repo.vwap_ask_price_signal_4 = cum_ask_amounts[4] / cum_ask_sizes[4];

    auto cum_mid_size_0 = (cum_bid_sizes[0] + cum_ask_sizes[0]) / 2.0;
    auto cum_mid_size_1 = (cum_bid_sizes[1] + cum_ask_sizes[1]) / 2.0;
    auto cum_mid_size_2 = (cum_bid_sizes[2] + cum_ask_sizes[2]) / 2.0;
    auto cum_mid_size_3 = (cum_bid_sizes[3] + cum_ask_sizes[3]) / 2.0;
    auto cum_mid_size_4 = (cum_bid_sizes[4] + cum_ask_sizes[4]) / 2.0;

    raw_price_repo.norm_vwap_bid_price_signal_0 = fill_price(current_bid_prices, current_bid_sizes, cum_mid_size_0);
    raw_price_repo.norm_vwap_bid_price_signal_1 = fill_price(current_bid_prices, current_bid_sizes, cum_mid_size_1);
    raw_price_repo.norm_vwap_bid_price_signal_2 = fill_price(current_bid_prices, current_bid_sizes, cum_mid_size_2);
    raw_price_repo.norm_vwap_bid_price_signal_3 = fill_price(current_bid_prices, current_bid_sizes, cum_mid_size_3);
    raw_price_repo.norm_vwap_bid_price_signal_4 = fill_price(current_bid_prices, current_bid_sizes, cum_mid_size_4);

    raw_price_repo.norm_vwap_ask_price_signal_0 = fill_price(current_ask_prices, current_ask_sizes, cum_mid_size_0);
    raw_price_repo.norm_vwap_ask_price_signal_1 = fill_price(current_ask_prices, current_ask_sizes, cum_mid_size_1);
    raw_price_repo.norm_vwap_ask_price_signal_2 = fill_price(current_ask_prices, current_ask_sizes, cum_mid_size_2);
    raw_price_repo.norm_vwap_ask_price_signal_3 = fill_price(current_ask_prices, current_ask_sizes, cum_mid_size_3);
    raw_price_repo.norm_vwap_ask_price_signal_4 = fill_price(current_ask_prices, current_ask_sizes, cum_mid_size_4);

    // compute micro signals
    raw_price_repo.micro_price_signal_0 = micro_price(raw_price_repo.norm_vwap_bid_price_signal_0,
                                                      raw_price_repo.norm_vwap_ask_price_signal_0,
                                                      cum_bid_sizes[0], cum_ask_sizes[0]);
    raw_price_repo.micro_price_signal_1 = micro_price(raw_price_repo.norm_vwap_bid_price_signal_1,
                                                      raw_price_repo.norm_vwap_ask_price_signal_1,
                                                      cum_bid_sizes[1], cum_ask_sizes[1]);
    raw_price_repo.micro_price_signal_2 = micro_price(raw_price_repo.norm_vwap_bid_price_signal_2,
                                                      raw_price_repo.norm_vwap_ask_price_signal_2,
                                                      cum_bid_sizes[2], cum_ask_sizes[2]);
    raw_price_repo.micro_price_signal_3 = micro_price(raw_price_repo.norm_vwap_bid_price_signal_3,
                                                      raw_price_repo.norm_vwap_ask_price_signal_3,
                                                      cum_bid_sizes[3], cum_ask_sizes[3]);
    raw_price_repo.micro_price_signal_4 = micro_price(raw_price_repo.norm_vwap_bid_price_signal_4,
                                                      raw_price_repo.norm_vwap_ask_price_signal_4,
                                                      cum_bid_sizes[4], cum_ask_sizes[4]);


    raw_price_repo.bid_fill_price_signal_0 = fill_price(current_bid_prices, current_bid_sizes, cum_ask_sizes[0]);
    raw_price_repo.bid_fill_price_signal_1 = fill_price(current_bid_prices, current_bid_sizes, cum_ask_sizes[1]);
    raw_price_repo.bid_fill_price_signal_2 = fill_price(current_bid_prices, current_bid_sizes, cum_ask_sizes[2]);
    raw_price_repo.bid_fill_price_signal_3 = fill_price(current_bid_prices, current_bid_sizes, cum_ask_sizes[3]);
    raw_price_repo.bid_fill_price_signal_4 = fill_price(current_bid_prices, current_bid_sizes, cum_ask_sizes[4]);

    raw_price_repo.ask_fill_price_signal_0 = fill_price(current_ask_prices, current_ask_sizes, cum_bid_sizes[0]);
    raw_price_repo.ask_fill_price_signal_1 = fill_price(current_ask_prices, current_ask_sizes, cum_bid_sizes[1]);
    raw_price_repo.ask_fill_price_signal_2 = fill_price(current_ask_prices, current_ask_sizes, cum_bid_sizes[2]);
    raw_price_repo.ask_fill_price_signal_3 = fill_price(current_ask_prices, current_ask_sizes, cum_bid_sizes[3]);
    raw_price_repo.ask_fill_price_signal_4 = fill_price(current_ask_prices, current_ask_sizes, cum_bid_sizes[4]);
}
