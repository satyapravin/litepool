#include "market_signal_builder.h"
#include <numeric>
#include <cmath>

#include "order.h"
#include "rl_macros.h"

using namespace RLTrader;

MarketSignalBuilder::MarketSignalBuilder(int depth)
              :previous_bid_prices(30, depth),
               previous_ask_prices(30, depth),
               previous_bid_amounts(30, depth),
               previous_ask_amounts(30, depth),
               previous_price_signal{},
               raw_price_diff_signals(std::make_unique<price_signal_repository>()),                      // price
               raw_spread_signals(std::make_unique<spread_signal_repository>()),                         // spread
               raw_volume_signals(std::make_unique<volume_signal_repository>())                          // volume
{
    
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
    compute_signals(book);

    std::vector<double> retval;
    insert_signals(retval, *raw_price_diff_signals);
    insert_signals(retval, *raw_spread_signals);
    insert_signals(retval, *raw_volume_signals);
    
    previous_bid_prices.addRow(book.bid_prices);
    previous_ask_prices.addRow(book.ask_prices);
    previous_bid_amounts.addRow(book.bid_sizes);
    previous_ask_amounts.addRow(book.ask_sizes);
    return retval;
}

void MarketSignalBuilder::compute_signals(Orderbook& book) {
    auto current_bid_prices = book.bid_prices;
    auto current_ask_prices = book.ask_prices;
    auto current_bid_sizes = book.bid_sizes;
    auto current_ask_sizes = book.ask_sizes;

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

    compute_spread_signals(repo);

    compute_volume_signals(current_bid_sizes, current_ask_sizes,
                           cum_bid_sizes, cum_ask_sizes);

    // here we compute OFI
    auto ofis = compute_lagged_ofi(repo, cum_bid_sizes, cum_ask_sizes, 1);
    raw_volume_signals->volume_ofi_signal_0 = std::get<0>(ofis);
    raw_volume_signals->volume_ofi_signal_1 = std::get<1>(ofis);

    ofis = compute_lagged_ofi(repo, cum_bid_sizes, cum_ask_sizes, 9);
    raw_volume_signals->volume_ofi_signal_2 = std::get<0>(ofis);
    raw_volume_signals->volume_ofi_signal_3 = std::get<1>(ofis);

    ofis = compute_lagged_ofi(repo, cum_bid_sizes, cum_ask_sizes, 19);
    raw_volume_signals->volume_ofi_signal_4 = std::get<0>(ofis);
    raw_volume_signals->volume_ofi_signal_5 = std::get<1>(ofis);

    ofis = compute_lagged_ofi(repo, cum_bid_sizes, cum_ask_sizes, 29);
    raw_volume_signals->volume_ofi_signal_6 = std::get<0>(ofis);
    raw_volume_signals->volume_ofi_signal_7 = std::get<1>(ofis);
}

std::tuple<double, double>
MarketSignalBuilder::compute_lagged_ofi(const price_signal_repository& repo,
                                        const std::vector<double>& cum_bid_sizes,
                                        const std::vector<double>& cum_ask_sizes,
                                        const int lag)
{
    auto current_bid_price = repo.vwap_bid_price_signal_0;
    auto current_ask_price = repo.vwap_ask_price_signal_0;
    auto current_bid_price_5 = repo.vwap_bid_price_signal_4;
    auto current_ask_price_5 = repo.vwap_ask_price_signal_4;
    auto current_bid_size = cum_bid_sizes[0];
    auto current_ask_size = cum_ask_sizes[0];
    auto current_bid_size_5 = cum_bid_sizes[5];
    auto current_ask_size_5 = cum_ask_sizes[5];

    auto lagged_bid_prices = previous_bid_prices.get(previous_bid_prices.get_lagged_row(lag));
    auto lagged_ask_prices = previous_ask_prices.get(previous_ask_prices.get_lagged_row(lag));
    auto lagged_bid_sizes = previous_bid_amounts.get(previous_bid_amounts.get_lagged_row(lag));
    auto lagged_ask_sizes = previous_ask_amounts.get(previous_ask_amounts.get_lagged_row(lag));

    std::vector<double> previous_cum_bid_sizes = get_cumulative_sizes(lagged_bid_sizes);
    std::vector<double> previous_cum_ask_sizes = get_cumulative_sizes(lagged_ask_sizes);

    std::vector<double> previous_cum_bid_amounts = cumulative_prod_sum(lagged_bid_prices, lagged_bid_sizes);
    std::vector<double> previous_cum_ask_amounts = cumulative_prod_sum(lagged_ask_prices, lagged_ask_sizes);

    double previous_vwap_bid_price_5 = previous_cum_bid_amounts[4] / previous_cum_bid_sizes[4];
    double previous_vwap_ask_price_5 = previous_cum_ask_amounts[4] / previous_cum_ask_sizes[4];

    double level_1 = compute_ofi(current_bid_price, current_bid_size,
                                 current_ask_price, current_ask_size,
                                 lagged_bid_prices[0], lagged_bid_sizes[0],
                                 lagged_ask_prices[0], lagged_ask_sizes[0]);

    double level_5 = compute_ofi(current_bid_price_5, current_bid_size_5,
                                 current_ask_price_5, current_ask_size_5,
                                 previous_vwap_bid_price_5, previous_cum_bid_sizes[4],
                                 previous_vwap_ask_price_5, previous_cum_ask_sizes[4]);
    return {level_1, level_5};
}

double MarketSignalBuilder::compute_ofi(double curr_bid_price, double curr_bid_size,
                                      double curr_ask_price, double curr_ask_size,
                                      double prev_bid_price, double prev_bid_size,
                                      double prev_ask_price, double prev_ask_size) {
    auto retval =  (curr_bid_price >= prev_bid_price ? curr_bid_size : 0) -
                         (curr_bid_price <= prev_bid_price ? prev_bid_size : 0) -
                         (curr_ask_price <= prev_ask_price ? curr_ask_size : 0) +
                         (curr_ask_price >= prev_ask_price ? prev_ask_size : 0);

    double normalizer = (curr_bid_size + curr_ask_size + prev_bid_size + prev_ask_size) * 0.5;
    return retval / normalizer;
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

    raw_price_diff_signals->mid_price_signal = raw_price_repo.mid_price_signal - previous_price_signal.mid_price_signal;
    raw_price_diff_signals->vwap_bid_price_signal_0 = raw_price_repo.vwap_bid_price_signal_0 - previous_price_signal.vwap_bid_price_signal_0;
    raw_price_diff_signals->vwap_bid_price_signal_1 = raw_price_repo.vwap_bid_price_signal_1 - previous_price_signal.vwap_bid_price_signal_1;
    raw_price_diff_signals->vwap_bid_price_signal_2 = raw_price_repo.vwap_bid_price_signal_2 - previous_price_signal.vwap_bid_price_signal_2;
    raw_price_diff_signals->vwap_bid_price_signal_3 = raw_price_repo.vwap_bid_price_signal_3 - previous_price_signal.vwap_bid_price_signal_3;
    raw_price_diff_signals->vwap_bid_price_signal_4 = raw_price_repo.vwap_bid_price_signal_4 - previous_price_signal.vwap_bid_price_signal_4;

    raw_price_diff_signals->vwap_ask_price_signal_0 = raw_price_repo.vwap_ask_price_signal_0 - previous_price_signal.vwap_ask_price_signal_0;
    raw_price_diff_signals->vwap_ask_price_signal_1 = raw_price_repo.vwap_ask_price_signal_1 - previous_price_signal.vwap_ask_price_signal_1;
    raw_price_diff_signals->vwap_ask_price_signal_2 = raw_price_repo.vwap_ask_price_signal_2 - previous_price_signal.vwap_ask_price_signal_2;
    raw_price_diff_signals->vwap_ask_price_signal_3 = raw_price_repo.vwap_ask_price_signal_3 - previous_price_signal.vwap_ask_price_signal_3;
    raw_price_diff_signals->vwap_ask_price_signal_4 = raw_price_repo.vwap_ask_price_signal_4 - previous_price_signal.vwap_ask_price_signal_4;

    raw_price_diff_signals->norm_vwap_bid_price_signal_0 = raw_price_repo.norm_vwap_bid_price_signal_0 - previous_price_signal.norm_vwap_bid_price_signal_0;
    raw_price_diff_signals->norm_vwap_bid_price_signal_1 = raw_price_repo.norm_vwap_bid_price_signal_1 - previous_price_signal.norm_vwap_bid_price_signal_1;
    raw_price_diff_signals->norm_vwap_bid_price_signal_2 = raw_price_repo.norm_vwap_bid_price_signal_2 - previous_price_signal.norm_vwap_bid_price_signal_2;
    raw_price_diff_signals->norm_vwap_bid_price_signal_3 = raw_price_repo.norm_vwap_bid_price_signal_3 - previous_price_signal.norm_vwap_bid_price_signal_3;
    raw_price_diff_signals->norm_vwap_bid_price_signal_4 = raw_price_repo.norm_vwap_bid_price_signal_4 - previous_price_signal.norm_vwap_bid_price_signal_4;

    raw_price_diff_signals->norm_vwap_ask_price_signal_0 = raw_price_repo.norm_vwap_ask_price_signal_0 - previous_price_signal.norm_vwap_ask_price_signal_0;
    raw_price_diff_signals->norm_vwap_ask_price_signal_1 = raw_price_repo.norm_vwap_ask_price_signal_1 - previous_price_signal.norm_vwap_ask_price_signal_1;
    raw_price_diff_signals->norm_vwap_ask_price_signal_2 = raw_price_repo.norm_vwap_ask_price_signal_2 - previous_price_signal.norm_vwap_ask_price_signal_2;
    raw_price_diff_signals->norm_vwap_ask_price_signal_3 = raw_price_repo.norm_vwap_ask_price_signal_3 - previous_price_signal.norm_vwap_ask_price_signal_3;
    raw_price_diff_signals->norm_vwap_ask_price_signal_4 = raw_price_repo.norm_vwap_ask_price_signal_4 - previous_price_signal.norm_vwap_ask_price_signal_4;

    raw_price_diff_signals->micro_price_signal_0 = raw_price_repo.micro_price_signal_0 - previous_price_signal.micro_price_signal_0;
    raw_price_diff_signals->micro_price_signal_1 = raw_price_repo.micro_price_signal_1 - previous_price_signal.micro_price_signal_1;
    raw_price_diff_signals->micro_price_signal_2 = raw_price_repo.micro_price_signal_2 - previous_price_signal.micro_price_signal_2;
    raw_price_diff_signals->micro_price_signal_3 = raw_price_repo.micro_price_signal_3 - previous_price_signal.micro_price_signal_3;
    raw_price_diff_signals->micro_price_signal_4 = raw_price_repo.micro_price_signal_4 - previous_price_signal.micro_price_signal_4;

    raw_price_diff_signals->bid_fill_price_signal_0 = raw_price_repo.bid_fill_price_signal_0 - previous_price_signal.bid_fill_price_signal_0;
    raw_price_diff_signals->bid_fill_price_signal_1 = raw_price_repo.bid_fill_price_signal_1 - previous_price_signal.bid_fill_price_signal_1;
    raw_price_diff_signals->bid_fill_price_signal_2 = raw_price_repo.bid_fill_price_signal_2 - previous_price_signal.bid_fill_price_signal_2;
    raw_price_diff_signals->bid_fill_price_signal_3 = raw_price_repo.bid_fill_price_signal_3 - previous_price_signal.bid_fill_price_signal_3;
    raw_price_diff_signals->bid_fill_price_signal_4 = raw_price_repo.bid_fill_price_signal_4 - previous_price_signal.bid_fill_price_signal_4;

    raw_price_diff_signals->ask_fill_price_signal_0 = raw_price_repo.ask_fill_price_signal_0 - previous_price_signal.ask_fill_price_signal_0;
    raw_price_diff_signals->ask_fill_price_signal_1 = raw_price_repo.ask_fill_price_signal_1 - previous_price_signal.ask_fill_price_signal_1;
    raw_price_diff_signals->ask_fill_price_signal_2 = raw_price_repo.ask_fill_price_signal_2 - previous_price_signal.ask_fill_price_signal_2;
    raw_price_diff_signals->ask_fill_price_signal_3 = raw_price_repo.ask_fill_price_signal_3 - previous_price_signal.ask_fill_price_signal_3;
    raw_price_diff_signals->ask_fill_price_signal_4 = raw_price_repo.ask_fill_price_signal_4 - previous_price_signal.ask_fill_price_signal_4;

    previous_price_signal = raw_price_repo;
}
