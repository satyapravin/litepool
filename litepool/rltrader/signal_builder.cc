#include "signal_builder.h"
#include <numeric>
#include <cmath>
#include <iostream>

#define NORMALIZE(struct_name, to_struct, mean_name, ssr_name, property_name, alpha) \
    do { \
        mean_name->property_name *= (1.0 - alpha); \
        mean_name->property_name += alpha * (struct_name.property_name - mean_name->property_name); \
        auto sqdelta = struct_name.property_name - mean_name->property_name; \
        ssr_name->property_name *= (1.0 - alpha); \
        ssr_name->property_name += alpha * sqdelta * sqdelta; \
        to_struct.property_name = sqdelta / std::pow(ssr_name->property_name + 1e-12, 0.5); \
    } while(0)

using namespace Simulator;

SignalBuilder::SignalBuilder(u_int bookhistory, u_int price_history)
              :processed_counter(0),
               minimum_required(bookhistory*price_history),
               bid_prices(bookhistory, 20),
               ask_prices(bookhistory, 20),
               bid_sizes(bookhistory, 20),
               ask_sizes(bookhistory, 20),
               raw_price_signals(price_history),                                                         // price
               raw_price_signal_mean(std::make_unique<price_signal_repository>()),                       // price mean
               raw_price_signal_ssqr(std::make_unique<price_signal_repository>()),                       // price ssqr
               raw_price_normalized_signals(std::make_unique<price_signal_repository>()),                // norm price
               raw_spread_signals(std::make_unique<spread_signal_repository>()),                         // spread
               raw_spread_signal_mean(std::make_unique<spread_signal_repository>()),                     // spread mean
               raw_spread_signal_ssqr(std::make_unique<spread_signal_repository>()),                     // spread ssqr
               raw_spread_normalized_signals(std::make_unique<spread_signal_repository>()),              // norm spread
               raw_volume_signals(std::make_unique<volume_signal_repository>()),                         // volume
               raw_volume_signal_mean(std::make_unique<volume_signal_repository>()),                     // volume mean
               raw_volume_signal_ssqr(std::make_unique<volume_signal_repository>()),                     // volume ssqr
               raw_volume_normalized_signals(std::make_unique<volume_signal_repository>()),              // norm volume
               raw_price_signal_velocities_1(std::make_unique<price_signal_repository>()),               // velocity
               raw_price_signal_velocity_1_mean(std::make_unique<price_signal_repository>()),            // velocity mean
               raw_price_signal_velocity_1_ssqr(std::make_unique<price_signal_repository>()),            // velocity ssqr
               raw_price_velocity_1_normalized_signals(std::make_unique<price_signal_repository>()),     // norm velocity
               raw_price_signal_velocities_10(std::make_unique<price_signal_repository>()),              // longterm velocity
               raw_price_signal_velocity_10_mean(std::make_unique<price_signal_repository>()),           // lonterm velocity mean
               raw_price_signal_velocity_10_ssqr(std::make_unique<price_signal_repository>()),           // longterm velocity ssqr
               raw_price_velocity_10_normalized_signals(std::make_unique<price_signal_repository>()),    // norm longterm velocity
               raw_price_vol_1_mean(std::make_unique<price_signal_repository>()),                        // longterm vol mean
               raw_price_vol_1_ssqr(std::make_unique<price_signal_repository>()),                        // longterm vol ssqr
               raw_price_vol_1_normalized_signals(std::make_unique<price_signal_repository>()),          // norm vol
               raw_price_vol_10_mean(std::make_unique<price_signal_repository>()),                       // longterm vol mean
               raw_price_vol_10_ssqr(std::make_unique<price_signal_repository>()),                       // longterm vol ssqr
               raw_price_vol_10_normalized_signals(std::make_unique<price_signal_repository>())          // norm longterm vol
{
}


bool SignalBuilder::is_data_ready() const {
    return processed_counter >= minimum_required;
}

double SignalBuilder::normalize(const double& raw, const double& alpha, double& mean, double& ssr) {
    mean *= 1.0 - alpha;
    mean += alpha * raw;
    ssr *= 1.0 - alpha;
    ssr += alpha * (raw - mean) * (raw - mean);
    return (raw - mean) / std::pow(ssr + 1e-7, 0.5);
}

price_signal_repository& SignalBuilder::get_price_signals(signal_type sigtype) {
    if (sigtype == signal_type::raw) {
        return raw_price_signals.get(0);
    } else if (sigtype == signal_type::norm) {
        return *raw_price_normalized_signals;
    } else if (sigtype == signal_type::mean) {
        return *raw_price_signal_mean;
    } else {
        return *raw_price_signal_ssqr;
    }
}

spread_signal_repository& SignalBuilder::get_spread_signals(signal_type sigtype) {
    if (sigtype == signal_type::raw) {
        return *raw_spread_signals;
    } else if (sigtype == signal_type::norm) {
        return *raw_spread_normalized_signals;
    } else if (sigtype == signal_type::mean) {
        return *raw_spread_signal_mean;
    } else {
        return *raw_spread_signal_ssqr;
    }
}

volume_signal_repository& SignalBuilder::get_volume_signals(signal_type sigtype) {
    if (sigtype == signal_type::raw) {
        return *raw_volume_signals;
    } else if (sigtype == signal_type::norm) {
        return *raw_volume_normalized_signals;
    } else if (sigtype == signal_type::mean) {
        return *raw_volume_signal_mean;
    } else {
        return *raw_volume_signal_ssqr;
    }
}
price_signal_repository& SignalBuilder::get_velocity_1_signals(signal_type sigtype) {
    if (sigtype == signal_type::raw) {
        return *raw_price_signal_velocities_1;
    } else if (sigtype == signal_type::norm) {
        return *raw_price_velocity_1_normalized_signals;
    } else if (sigtype == signal_type::mean) {
        return *raw_price_signal_velocity_1_mean;
    } else {
        return *raw_price_signal_velocity_1_ssqr;
    }
}

price_signal_repository& SignalBuilder::get_velocity_10_signals(signal_type sigtype) {
if (sigtype == signal_type::raw) {
        return *raw_price_signal_velocities_10;
    } else if (sigtype == signal_type::norm) {
        return *raw_price_velocity_10_normalized_signals;
    } else if (sigtype == signal_type::mean) {
        return *raw_price_signal_velocity_10_mean;
    } else {
        return *raw_price_signal_velocity_10_ssqr;
    }
}

price_signal_repository& SignalBuilder::get_volatility_1_signals(signal_type sigtype) {
    if (sigtype == signal_type::raw) {
        return *raw_price_signal_velocity_1_ssqr;
    } else if (sigtype == signal_type::norm) {
        return *raw_price_vol_1_normalized_signals;
    } else if (sigtype == signal_type::mean) {
        return *raw_price_vol_1_mean;
    } else {
        return *raw_price_vol_1_ssqr;
    }
}

price_signal_repository& SignalBuilder::get_volatility_10_signals(signal_type sigtype) {
    if (sigtype == signal_type::raw) {
        return *raw_price_signal_velocity_10_ssqr;
    } else if (sigtype == signal_type::norm) {
        return *raw_price_vol_10_normalized_signals;
    } else if (sigtype == signal_type::mean) {
        return *raw_price_vol_10_mean;
    } else {
        return *raw_price_vol_10_ssqr;
    }
}

template<typename T>
void insert_signals(std::vector<double>& signals, T& repo) {
    size_t num_doubles = sizeof(T) / sizeof(double);
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Waddress-of-packed-member"
    double* start = reinterpret_cast<double*>(&repo);
    #pragma GCC diagnostic pop
    double* end = start + num_doubles;
    signals.insert(signals.end(), start, end);
}

void SignalBuilder::insert_norm_price_signals(std::vector<double>& signals) {
    insert_signals(signals, *raw_price_normalized_signals);
}

void SignalBuilder::insert_norm_spread_signals(std::vector<double>& signals) {
    insert_signals(signals, *raw_spread_normalized_signals);
}

void SignalBuilder::insert_norm_volume_signals(std::vector<double>& signals) {
    insert_signals(signals, *raw_volume_normalized_signals);
}

void SignalBuilder::insert_norm_velocity_1_signals(std::vector<double>& signals) {
    insert_signals(signals, *raw_price_velocity_1_normalized_signals);
}

void SignalBuilder::insert_norm_velocity_10_signals(std::vector<double>& signals) {
    insert_signals(signals, *raw_price_velocity_10_normalized_signals);
}

void SignalBuilder::insert_norm_volatility_1_signals(std::vector<double>& signals) {
    insert_signals(signals, *raw_price_vol_1_normalized_signals);
}

void SignalBuilder::insert_norm_volatility_10_signals(std::vector<double>& signals) {
    insert_signals(signals, *raw_price_vol_10_normalized_signals);
}


std::vector<double> cumulative_prod_sum(std::vector<double>& first, std::vector<double>& second) {
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

std::vector<double> SignalBuilder::add_book(Orderbook& book) {
    bid_prices.addRow(book.bid_prices);
    bid_sizes.addRow(book.bid_sizes);
    ask_prices.addRow(book.ask_prices);
    ask_sizes.addRow(book.ask_sizes);

    compute_signals();

    std::vector<double> retval;

    if (is_data_ready()) {
        insert_norm_price_signals(retval);
        insert_norm_spread_signals(retval);
        insert_norm_volume_signals(retval);
        insert_norm_velocity_1_signals(retval);
        insert_norm_velocity_10_signals(retval);
        insert_norm_volatility_1_signals(retval);
        insert_norm_volatility_10_signals(retval);
    }

    return retval;
}

void SignalBuilder::compute_signals() {
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

    if (processed_counter >= 10) {
        compute_velocity_signals();
        normalize_signals();
        compute_volatility_signals();
        normalize_volatility_1_signals();
        normalize_volatility_10_signals();
    }

    processed_counter++;
}

void SignalBuilder::normalize_signals() {
    normalize_price_signals();
    normalize_spread_signals();
    normalize_volume_signals();
    normalize_velocity_1_signals();
    normalize_velocity_10_signals();
}

void SignalBuilder::normalize_volatility_1_signals() {
    auto& vol_1_sig = *raw_price_signal_velocity_1_ssqr;
    auto& to = *raw_price_vol_1_normalized_signals;
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, mid_price_signal, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, vwap_bid_price_signal_0, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, vwap_bid_price_signal_1, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, vwap_bid_price_signal_2, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, vwap_bid_price_signal_3, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, vwap_bid_price_signal_4, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, vwap_ask_price_signal_0, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, vwap_ask_price_signal_1, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, vwap_ask_price_signal_2, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, vwap_ask_price_signal_3, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, vwap_ask_price_signal_4, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, norm_vwap_bid_price_signal_0, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, norm_vwap_bid_price_signal_1, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, norm_vwap_bid_price_signal_2, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, norm_vwap_bid_price_signal_3, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, norm_vwap_bid_price_signal_4, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, norm_vwap_ask_price_signal_0, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, norm_vwap_ask_price_signal_1, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, norm_vwap_ask_price_signal_2, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, norm_vwap_ask_price_signal_3, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, norm_vwap_ask_price_signal_4, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, micro_price_signal_0, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, micro_price_signal_1, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, micro_price_signal_2, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, micro_price_signal_3, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, micro_price_signal_4, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, bid_fill_price_signal_0, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, bid_fill_price_signal_1, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, bid_fill_price_signal_2, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, bid_fill_price_signal_3, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, bid_fill_price_signal_4, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, ask_fill_price_signal_0, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, ask_fill_price_signal_1, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, ask_fill_price_signal_2, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, ask_fill_price_signal_3, norm_alpha);
    NORMALIZE(vol_1_sig, to, raw_price_vol_1_mean, raw_price_vol_1_ssqr, ask_fill_price_signal_4, norm_alpha);
}

void SignalBuilder::normalize_volatility_10_signals() {
    auto& vol_10_sig = *raw_price_signal_velocity_10_ssqr;
    auto& to = *raw_price_vol_10_normalized_signals;
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, mid_price_signal, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, vwap_bid_price_signal_0, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, vwap_bid_price_signal_1, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, vwap_bid_price_signal_2, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, vwap_bid_price_signal_3, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, vwap_bid_price_signal_4, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, vwap_ask_price_signal_0, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, vwap_ask_price_signal_1, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, vwap_ask_price_signal_2, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, vwap_ask_price_signal_3, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, vwap_ask_price_signal_4, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, norm_vwap_bid_price_signal_0, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, norm_vwap_bid_price_signal_1, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, norm_vwap_bid_price_signal_2, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, norm_vwap_bid_price_signal_3, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, norm_vwap_bid_price_signal_4, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, norm_vwap_ask_price_signal_0, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, norm_vwap_ask_price_signal_1, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, norm_vwap_ask_price_signal_2, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, norm_vwap_ask_price_signal_3, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, norm_vwap_ask_price_signal_4, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, micro_price_signal_0, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, micro_price_signal_1, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, micro_price_signal_2, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, micro_price_signal_3, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, micro_price_signal_4, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, bid_fill_price_signal_0, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, bid_fill_price_signal_1, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, bid_fill_price_signal_2, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, bid_fill_price_signal_3, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, bid_fill_price_signal_4, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, ask_fill_price_signal_0, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, ask_fill_price_signal_1, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, ask_fill_price_signal_2, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, ask_fill_price_signal_3, norm_alpha);
    NORMALIZE(vol_10_sig, to, raw_price_vol_10_mean, raw_price_vol_10_ssqr, ask_fill_price_signal_4, norm_alpha);
}

void SignalBuilder::normalize_velocity_1_signals() {
    auto& velocity_1_signal = *raw_price_signal_velocities_1;
    auto& to = *raw_price_velocity_1_normalized_signals;
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, mid_price_signal, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, vwap_bid_price_signal_0, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, vwap_bid_price_signal_1, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, vwap_bid_price_signal_2, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, vwap_bid_price_signal_3, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, vwap_bid_price_signal_4, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, vwap_ask_price_signal_0, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, vwap_ask_price_signal_1, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, vwap_ask_price_signal_2, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, vwap_ask_price_signal_3, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, vwap_ask_price_signal_4, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, norm_vwap_bid_price_signal_0, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, norm_vwap_bid_price_signal_1, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, norm_vwap_bid_price_signal_2, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, norm_vwap_bid_price_signal_3, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, norm_vwap_bid_price_signal_4, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, norm_vwap_ask_price_signal_0, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, norm_vwap_ask_price_signal_1, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, norm_vwap_ask_price_signal_2, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, norm_vwap_ask_price_signal_3, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, norm_vwap_ask_price_signal_4, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, micro_price_signal_0, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, micro_price_signal_1, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, micro_price_signal_2, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, micro_price_signal_3, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, micro_price_signal_4, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, bid_fill_price_signal_0, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, bid_fill_price_signal_1, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, bid_fill_price_signal_2, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, bid_fill_price_signal_3, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, bid_fill_price_signal_4, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, ask_fill_price_signal_0, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, ask_fill_price_signal_1, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, ask_fill_price_signal_2, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, ask_fill_price_signal_3, norm_alpha);
    NORMALIZE(velocity_1_signal, to, raw_price_signal_velocity_1_mean, raw_price_signal_velocity_1_ssqr, ask_fill_price_signal_4, norm_alpha);
}


void SignalBuilder::normalize_velocity_10_signals() {
    auto& velo_10_sig = *raw_price_signal_velocities_10;
    auto& to = *raw_price_velocity_10_normalized_signals;
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, mid_price_signal, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, vwap_bid_price_signal_0, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, vwap_bid_price_signal_1, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, vwap_bid_price_signal_2, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, vwap_bid_price_signal_3, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, vwap_bid_price_signal_4, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, vwap_ask_price_signal_0, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, vwap_ask_price_signal_1, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, vwap_ask_price_signal_2, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, vwap_ask_price_signal_3, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, vwap_ask_price_signal_4, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, norm_vwap_bid_price_signal_0, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, norm_vwap_bid_price_signal_1, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, norm_vwap_bid_price_signal_2, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, norm_vwap_bid_price_signal_3, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, norm_vwap_bid_price_signal_4, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, norm_vwap_ask_price_signal_0, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, norm_vwap_ask_price_signal_1, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, norm_vwap_ask_price_signal_2, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, norm_vwap_ask_price_signal_3, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, norm_vwap_ask_price_signal_4, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, micro_price_signal_0, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, micro_price_signal_1, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, micro_price_signal_2, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, micro_price_signal_3, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, micro_price_signal_4, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, bid_fill_price_signal_0, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, bid_fill_price_signal_1, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, bid_fill_price_signal_2, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, bid_fill_price_signal_3, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, bid_fill_price_signal_4, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, ask_fill_price_signal_0, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, ask_fill_price_signal_1, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, ask_fill_price_signal_2, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, ask_fill_price_signal_3, norm_alpha);
    NORMALIZE(velo_10_sig, to, raw_price_signal_velocity_10_mean, raw_price_signal_velocity_10_ssqr, ask_fill_price_signal_4, norm_alpha);
}

void SignalBuilder::normalize_volume_signals() {
    auto& volume_signal = *raw_volume_signals;
    auto& to = *raw_volume_normalized_signals;
    NORMALIZE(volume_signal, to, raw_volume_signal_mean, raw_volume_signal_ssqr, volume_imbalance_signal_0, norm_alpha);
    NORMALIZE(volume_signal, to, raw_volume_signal_mean, raw_volume_signal_ssqr, volume_imbalance_signal_1, norm_alpha);
    NORMALIZE(volume_signal, to, raw_volume_signal_mean, raw_volume_signal_ssqr, volume_imbalance_signal_2, norm_alpha);
    NORMALIZE(volume_signal, to, raw_volume_signal_mean, raw_volume_signal_ssqr, volume_imbalance_signal_3, norm_alpha);
    NORMALIZE(volume_signal, to, raw_volume_signal_mean, raw_volume_signal_ssqr, volume_imbalance_signal_4, norm_alpha);
}


void SignalBuilder::normalize_spread_signals() {
    auto& spread_signal = *raw_spread_signals;
    auto& to = *raw_spread_normalized_signals;
    NORMALIZE(spread_signal, to, raw_spread_signal_mean, raw_spread_signal_ssqr, norm_vwap_bid_spread_signal_0, norm_alpha);
    NORMALIZE(spread_signal, to, raw_spread_signal_mean, raw_spread_signal_ssqr, norm_vwap_bid_spread_signal_1, norm_alpha);
    NORMALIZE(spread_signal, to, raw_spread_signal_mean, raw_spread_signal_ssqr, norm_vwap_bid_spread_signal_2, norm_alpha);
    NORMALIZE(spread_signal, to, raw_spread_signal_mean, raw_spread_signal_ssqr, norm_vwap_bid_spread_signal_3, norm_alpha);
    NORMALIZE(spread_signal, to, raw_spread_signal_mean, raw_spread_signal_ssqr, norm_vwap_bid_spread_signal_4, norm_alpha);
    NORMALIZE(spread_signal, to, raw_spread_signal_mean, raw_spread_signal_ssqr, norm_vwap_ask_spread_signal_0, norm_alpha);
    NORMALIZE(spread_signal, to, raw_spread_signal_mean, raw_spread_signal_ssqr, norm_vwap_ask_spread_signal_1, norm_alpha);
    NORMALIZE(spread_signal, to, raw_spread_signal_mean, raw_spread_signal_ssqr, norm_vwap_ask_spread_signal_2, norm_alpha);
    NORMALIZE(spread_signal, to, raw_spread_signal_mean, raw_spread_signal_ssqr, norm_vwap_ask_spread_signal_3, norm_alpha);
    NORMALIZE(spread_signal, to, raw_spread_signal_mean, raw_spread_signal_ssqr, norm_vwap_ask_spread_signal_4, norm_alpha);
    NORMALIZE(spread_signal, to, raw_spread_signal_mean, raw_spread_signal_ssqr, micro_spread_signal_0, norm_alpha);
    NORMALIZE(spread_signal, to, raw_spread_signal_mean, raw_spread_signal_ssqr, micro_spread_signal_1, norm_alpha);
    NORMALIZE(spread_signal, to, raw_spread_signal_mean, raw_spread_signal_ssqr, micro_spread_signal_2, norm_alpha);
    NORMALIZE(spread_signal, to, raw_spread_signal_mean, raw_spread_signal_ssqr, micro_spread_signal_3, norm_alpha);
    NORMALIZE(spread_signal, to, raw_spread_signal_mean, raw_spread_signal_ssqr, micro_spread_signal_4, norm_alpha);
    NORMALIZE(spread_signal, to, raw_spread_signal_mean, raw_spread_signal_ssqr, bid_fill_spread_signal_0, norm_alpha);
    NORMALIZE(spread_signal, to, raw_spread_signal_mean, raw_spread_signal_ssqr, bid_fill_spread_signal_1, norm_alpha);
    NORMALIZE(spread_signal, to, raw_spread_signal_mean, raw_spread_signal_ssqr, bid_fill_spread_signal_2, norm_alpha);
    NORMALIZE(spread_signal, to, raw_spread_signal_mean, raw_spread_signal_ssqr, bid_fill_spread_signal_3, norm_alpha);
    NORMALIZE(spread_signal, to, raw_spread_signal_mean, raw_spread_signal_ssqr, bid_fill_spread_signal_4, norm_alpha);
    NORMALIZE(spread_signal, to, raw_spread_signal_mean, raw_spread_signal_ssqr, ask_fill_spread_signal_0, norm_alpha);
    NORMALIZE(spread_signal, to, raw_spread_signal_mean, raw_spread_signal_ssqr, ask_fill_spread_signal_1, norm_alpha);
    NORMALIZE(spread_signal, to, raw_spread_signal_mean, raw_spread_signal_ssqr, ask_fill_spread_signal_2, norm_alpha);
    NORMALIZE(spread_signal, to, raw_spread_signal_mean, raw_spread_signal_ssqr, ask_fill_spread_signal_3, norm_alpha);
    NORMALIZE(spread_signal, to, raw_spread_signal_mean, raw_spread_signal_ssqr, ask_fill_spread_signal_4, norm_alpha);
}

void SignalBuilder::normalize_price_signals() {
    auto& price_signal = raw_price_signals.get(0);
    auto& to = *raw_price_normalized_signals;
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, mid_price_signal, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, vwap_bid_price_signal_0, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, vwap_bid_price_signal_1, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, vwap_bid_price_signal_2, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, vwap_bid_price_signal_3, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, vwap_bid_price_signal_4, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, vwap_ask_price_signal_0, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, vwap_ask_price_signal_1, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, vwap_ask_price_signal_2, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, vwap_ask_price_signal_3, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, vwap_ask_price_signal_4, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, norm_vwap_bid_price_signal_0, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, norm_vwap_bid_price_signal_1, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, norm_vwap_bid_price_signal_2, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, norm_vwap_bid_price_signal_3, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, norm_vwap_bid_price_signal_4, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, norm_vwap_ask_price_signal_0, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, norm_vwap_ask_price_signal_1, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, norm_vwap_ask_price_signal_2, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, norm_vwap_ask_price_signal_3, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, norm_vwap_ask_price_signal_4, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, micro_price_signal_0, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, micro_price_signal_1, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, micro_price_signal_2, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, micro_price_signal_3, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, micro_price_signal_4, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, bid_fill_price_signal_0, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, bid_fill_price_signal_1, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, bid_fill_price_signal_2, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, bid_fill_price_signal_3, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, bid_fill_price_signal_4, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, ask_fill_price_signal_0, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, ask_fill_price_signal_1, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, ask_fill_price_signal_2, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, ask_fill_price_signal_3, norm_alpha);
    NORMALIZE(price_signal, to, raw_price_signal_mean, raw_price_signal_ssqr, ask_fill_price_signal_4, norm_alpha);
}

void diff(price_signal_repository& result, const price_signal_repository&current, const price_signal_repository& prev) {
    result.mid_price_signal = current.mid_price_signal - prev.mid_price_signal;

    result.vwap_bid_price_signal_0 = current.vwap_bid_price_signal_0 - prev.vwap_bid_price_signal_0;
    result.vwap_bid_price_signal_1 = current.vwap_bid_price_signal_1 - prev.vwap_bid_price_signal_1;
    result.vwap_bid_price_signal_2 = current.vwap_bid_price_signal_2 - prev.vwap_bid_price_signal_2;
    result.vwap_bid_price_signal_3 = current.vwap_bid_price_signal_3 - prev.vwap_bid_price_signal_3;
    result.vwap_bid_price_signal_4 = current.vwap_bid_price_signal_4 - prev.vwap_bid_price_signal_4;

    result.vwap_ask_price_signal_0 = current.vwap_ask_price_signal_0 - prev.vwap_ask_price_signal_0;
    result.vwap_ask_price_signal_1 = current.vwap_ask_price_signal_1 - prev.vwap_ask_price_signal_1;
    result.vwap_ask_price_signal_2 = current.vwap_ask_price_signal_2 - prev.vwap_ask_price_signal_2;
    result.vwap_ask_price_signal_3 = current.vwap_ask_price_signal_3 - prev.vwap_ask_price_signal_3;
    result.vwap_ask_price_signal_4 = current.vwap_ask_price_signal_4 - prev.vwap_ask_price_signal_4;

    result.norm_vwap_bid_price_signal_0 = current.norm_vwap_bid_price_signal_0 - prev.norm_vwap_bid_price_signal_0;
    result.norm_vwap_bid_price_signal_1 = current.norm_vwap_bid_price_signal_1 - prev.norm_vwap_bid_price_signal_1;
    result.norm_vwap_bid_price_signal_2 = current.norm_vwap_bid_price_signal_2 - prev.norm_vwap_bid_price_signal_2;
    result.norm_vwap_bid_price_signal_3 = current.norm_vwap_bid_price_signal_3 - prev.norm_vwap_bid_price_signal_3;
    result.norm_vwap_bid_price_signal_4 = current.norm_vwap_bid_price_signal_4 - prev.norm_vwap_bid_price_signal_4;

    result.norm_vwap_ask_price_signal_0 = current.norm_vwap_ask_price_signal_0 - prev.norm_vwap_ask_price_signal_0;
    result.norm_vwap_ask_price_signal_1 = current.norm_vwap_ask_price_signal_1 - prev.norm_vwap_ask_price_signal_1;
    result.norm_vwap_ask_price_signal_2 = current.norm_vwap_ask_price_signal_2 - prev.norm_vwap_ask_price_signal_2;
    result.norm_vwap_ask_price_signal_3 = current.norm_vwap_ask_price_signal_3 - prev.norm_vwap_ask_price_signal_3;
    result.norm_vwap_ask_price_signal_4 = current.norm_vwap_ask_price_signal_4 - prev.norm_vwap_ask_price_signal_4;

    result.micro_price_signal_0 = current.micro_price_signal_0 - prev.micro_price_signal_0;
    result.micro_price_signal_1 = current.micro_price_signal_1 - prev.micro_price_signal_1;
    result.micro_price_signal_2 = current.micro_price_signal_2 - prev.micro_price_signal_2;
    result.micro_price_signal_3 = current.micro_price_signal_3 - prev.micro_price_signal_3;
    result.micro_price_signal_4 = current.micro_price_signal_4 - prev.micro_price_signal_4;

    result.bid_fill_price_signal_0 = current.bid_fill_price_signal_0 - prev.bid_fill_price_signal_0;
    result.bid_fill_price_signal_1 = current.bid_fill_price_signal_1 - prev.bid_fill_price_signal_1;
    result.bid_fill_price_signal_2 = current.bid_fill_price_signal_2 - prev.bid_fill_price_signal_2;
    result.bid_fill_price_signal_3 = current.bid_fill_price_signal_3 - prev.bid_fill_price_signal_3;
    result.bid_fill_price_signal_4 = current.bid_fill_price_signal_4 - prev.bid_fill_price_signal_4;

    result.ask_fill_price_signal_0 = current.ask_fill_price_signal_0 - prev.ask_fill_price_signal_0;
    result.ask_fill_price_signal_1 = current.ask_fill_price_signal_1 - prev.ask_fill_price_signal_1;
    result.ask_fill_price_signal_2 = current.ask_fill_price_signal_2 - prev.ask_fill_price_signal_2;
    result.ask_fill_price_signal_3 = current.ask_fill_price_signal_3 - prev.ask_fill_price_signal_3;
    result.ask_fill_price_signal_4 = current.ask_fill_price_signal_4 - prev.ask_fill_price_signal_4;
}

void SignalBuilder::compute_volatility_signals() {

}

void SignalBuilder::compute_velocity_signals() {
    auto current = raw_price_signals.get(0);
    auto prev01 = raw_price_signals.get(1);
    auto prev10 = raw_price_signals.get(10);
    price_signal_repository& vel01 = *raw_price_signal_velocities_1;
    price_signal_repository& vel10 = *raw_price_signal_velocities_10;
    diff(vel01, current, prev01);
    diff(vel10, current, prev10);
}


void SignalBuilder::compute_volume_signals(std::vector<double>& bid_sizes,
                                           std::vector<double>& ask_sizes,
                                           std::vector<double>& cum_bid_sizes,
                                           std::vector<double>& cum_ask_sizes) {
    raw_volume_signals->volume_imbalance_signal_0 = (bid_sizes[0] - ask_sizes[0]) / (bid_sizes[0] + ask_sizes[0]);
    raw_volume_signals->volume_imbalance_signal_1 = (cum_bid_sizes[1] - cum_ask_sizes[1]) / (cum_bid_sizes[1] + cum_ask_sizes[1]);
    raw_volume_signals->volume_imbalance_signal_2 = (cum_bid_sizes[2] - cum_ask_sizes[2]) / (cum_bid_sizes[2] + cum_ask_sizes[2]);
    raw_volume_signals->volume_imbalance_signal_3 = (cum_bid_sizes[3] - cum_ask_sizes[3]) / (cum_bid_sizes[3] + cum_ask_sizes[3]);
    raw_volume_signals->volume_imbalance_signal_4 = (cum_bid_sizes[4] - cum_ask_sizes[4]) / (cum_bid_sizes[4] + cum_ask_sizes[4]);
}

void SignalBuilder::compute_spread_signals(price_signal_repository& repo) {
    raw_spread_signals->norm_vwap_bid_spread_signal_0 = repo.norm_vwap_bid_price_signal_0 - repo.mid_price_signal;
    raw_spread_signals->norm_vwap_bid_spread_signal_1 = repo.norm_vwap_bid_price_signal_1 - repo.mid_price_signal;
    raw_spread_signals->norm_vwap_bid_spread_signal_2 = repo.norm_vwap_bid_price_signal_2 - repo.mid_price_signal;
    raw_spread_signals->norm_vwap_bid_spread_signal_3 = repo.norm_vwap_bid_price_signal_3 - repo.mid_price_signal;
    raw_spread_signals->norm_vwap_bid_spread_signal_4 = repo.norm_vwap_bid_price_signal_4 - repo.mid_price_signal;

    raw_spread_signals->norm_vwap_ask_spread_signal_0 = repo.norm_vwap_ask_price_signal_0 - repo.mid_price_signal;
    raw_spread_signals->norm_vwap_ask_spread_signal_1 = repo.norm_vwap_ask_price_signal_1 - repo.mid_price_signal;
    raw_spread_signals->norm_vwap_ask_spread_signal_2 = repo.norm_vwap_ask_price_signal_2 - repo.mid_price_signal;
    raw_spread_signals->norm_vwap_ask_spread_signal_3 = repo.norm_vwap_ask_price_signal_3 - repo.mid_price_signal;
    raw_spread_signals->norm_vwap_ask_spread_signal_4 = repo.norm_vwap_ask_price_signal_4 - repo.mid_price_signal;

    raw_spread_signals->micro_spread_signal_0 = repo.micro_price_signal_0 - repo.mid_price_signal;
    raw_spread_signals->micro_spread_signal_1 = repo.micro_price_signal_1 - repo.mid_price_signal;
    raw_spread_signals->micro_spread_signal_2 = repo.micro_price_signal_2 - repo.mid_price_signal;
    raw_spread_signals->micro_spread_signal_3 = repo.micro_price_signal_3 - repo.mid_price_signal;
    raw_spread_signals->micro_spread_signal_4 = repo.micro_price_signal_4 - repo.mid_price_signal;

    raw_spread_signals->bid_fill_spread_signal_0 = repo.bid_fill_price_signal_0 - repo.mid_price_signal;
    raw_spread_signals->bid_fill_spread_signal_1 = repo.bid_fill_price_signal_1 - repo.mid_price_signal;
    raw_spread_signals->bid_fill_spread_signal_2 = repo.bid_fill_price_signal_2 - repo.mid_price_signal;
    raw_spread_signals->bid_fill_spread_signal_3 = repo.bid_fill_price_signal_3 - repo.mid_price_signal;
    raw_spread_signals->bid_fill_spread_signal_4 = repo.bid_fill_price_signal_4 - repo.mid_price_signal;

    raw_spread_signals->ask_fill_spread_signal_0 = repo.ask_fill_price_signal_0 - repo.mid_price_signal;
    raw_spread_signals->ask_fill_spread_signal_1 = repo.ask_fill_price_signal_1 - repo.mid_price_signal;
    raw_spread_signals->ask_fill_spread_signal_2 = repo.ask_fill_price_signal_2 - repo.mid_price_signal;
    raw_spread_signals->ask_fill_spread_signal_3 = repo.ask_fill_price_signal_3 - repo.mid_price_signal;
    raw_spread_signals->ask_fill_spread_signal_4 = repo.ask_fill_price_signal_4 - repo.mid_price_signal;
}

void SignalBuilder::compute_price_signals(price_signal_repository& raw_price_repo,
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
