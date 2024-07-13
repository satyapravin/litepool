#include "trade_signal_builder.h"
#include "norm_macro.h"

using namespace Simulator;

TradeSignalBuilder::TradeSignalBuilder()
                   :raw_signals(10)
                   , mean_raw_signals(std::make_unique<trade_signal_repository>())
                   , ssr_raw_signals(std::make_unique<trade_signal_repository>())
                   , norm_raw_signals(std::make_unique<trade_signal_repository>())
                   , velocity_10_signals(std::make_unique<trade_signal_repository>())
                   , mean_velocity_10_signals(std::make_unique<trade_signal_repository>())
                   , ssr_velocity_10_signals(std::make_unique<trade_signal_repository>())
                   , norm_velocity_10_signals(std::make_unique<trade_signal_repository>())
                   , mean_volatility_10_signals(std::make_unique<trade_signal_repository>())
                   , ssr_volatility_10_signals(std::make_unique<trade_signal_repository>())
                   , norm_volatility_10_signals(std::make_unique<trade_signal_repository>())
{
}


void TradeSignalBuilder::compute_trade_signals(TradeInfo& info, double& bid_price, double& ask_price) {
    trade_signal_repository repo;
    auto gross_trade = info.buy_trades + info.sell_trades;
    auto gross_amount = info.buy_amount + info.sell_amount;
    repo.average_trade_amount = gross_amount / gross_trade;
    repo.buy_amount_ratio = info.buy_amount / gross_amount;
    repo.sell_amount_ratio = info.sell_amount / gross_amount;
    repo.number_of_trades = gross_trade;
    repo.relative_buy_price = info.average_buy_price / ask_price;
    repo.relative_sell_price = info.average_sell_price / bid_price;
    repo.buy_num_trade_ratio = info.buy_trades / gross_trade;
    repo.sell_num_trade_ratio = info.sell_trades / gross_trade;
    this->raw_signals.add(repo);
    auto& to = *norm_raw_signals;
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, average_trade_amount, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, buy_amount_ratio, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, sell_amount_ratio, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, number_of_trades, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, relative_buy_price, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, relative_sell_price, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, buy_num_trade_ratio, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, sell_num_trade_ratio, alpha);
}

void TradeSignalBuilder::compute_velocity_signals() {
    auto& current = raw_signals.get(0);
    auto& prev10 = raw_signals.get(10);
    trade_signal_repository& vel10 = *velocity_10_signals;
    vel10.average_trade_amount = current.average_trade_amount - prev10.average_trade_amount;
    vel10.buy_amount_ratio = current.buy_amount_ratio - prev10.buy_amount_ratio;
    vel10.sell_amount_ratio = current.sell_amount_ratio - prev10.sell_amount_ratio;
    vel10.number_of_trades = current.number_of_trades - prev10.number_of_trades;
    vel10.relative_buy_price = current.relative_buy_price - prev10.relative_buy_price;
    vel10.relative_sell_price = current.relative_sell_price - prev10.relative_sell_price;
    vel10.buy_num_trade_ratio = current.buy_num_trade_ratio - prev10.buy_num_trade_ratio;
    vel10.sell_num_trade_ratio = current.sell_num_trade_ratio - prev10.sell_num_trade_ratio;
    auto& to = *norm_velocity_10_signals;
    NORMALIZE(vel10, to, mean_raw_signals, ssr_raw_signals, average_trade_amount, alpha);
    NORMALIZE(vel10, to, mean_raw_signals, ssr_raw_signals, buy_amount_ratio, alpha);
    NORMALIZE(vel10, to, mean_raw_signals, ssr_raw_signals, sell_amount_ratio, alpha);
    NORMALIZE(vel10, to, mean_raw_signals, ssr_raw_signals, number_of_trades, alpha);
    NORMALIZE(vel10, to, mean_raw_signals, ssr_raw_signals, relative_buy_price, alpha);
    NORMALIZE(vel10, to, mean_raw_signals, ssr_raw_signals, relative_sell_price, alpha);
    NORMALIZE(vel10, to, mean_raw_signals, ssr_raw_signals, buy_num_trade_ratio, alpha);
    NORMALIZE(vel10, to, mean_raw_signals, ssr_raw_signals, sell_num_trade_ratio, alpha);
}

void TradeSignalBuilder::compute_volatility_signals() {
    auto& repo = *ssr_velocity_10_signals;
    auto& to = *norm_volatility_10_signals;
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, average_trade_amount, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, buy_amount_ratio, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, sell_amount_ratio, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, number_of_trades, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, relative_buy_price, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, relative_sell_price, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, buy_num_trade_ratio, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, sell_num_trade_ratio, alpha);
}

std::vector<double> TradeSignalBuilder::add_trade(TradeInfo& info, double& bid_price, double& ask_price) {
    compute_trade_signals(info, bid_price, ask_price);

    if (processed_counter > 10) {
        compute_velocity_signals();
        compute_volatility_signals();
    }

    std::vector<double> retval;
    insert_signals(retval, *norm_raw_signals);
    insert_signals(retval, *norm_velocity_10_signals);
    insert_signals(retval, *norm_volatility_10_signals);
    processed_counter++;
    return retval;
}

trade_signal_repository& TradeSignalBuilder::get_trade_signals(signal_type sigtype) {
    if (sigtype == signal_type::raw)
        return raw_signals.get(0);
    else if (sigtype == signal_type::mean)
        return *mean_raw_signals;
    else if (sigtype == signal_type::ssr)
        return *ssr_raw_signals;
    else if (sigtype == signal_type::norm)
        return *norm_raw_signals;
    else
        throw std::runtime_error("Invalid signal type");
}

trade_signal_repository& TradeSignalBuilder::get_velocity_signals(signal_type sigtype) {
    if (sigtype == signal_type::raw)
        return *velocity_10_signals;
    else if (sigtype == signal_type::mean)
        return *mean_velocity_10_signals;
    else if (sigtype == signal_type::ssr)
        return *ssr_velocity_10_signals;
    else if (sigtype == signal_type::norm)
        return *norm_velocity_10_signals;
    else
        throw std::runtime_error("Invalid signal type");
}

trade_signal_repository& TradeSignalBuilder::get_volatility_signals(signal_type sigtype) {
    if (sigtype == signal_type::raw)
        return *ssr_velocity_10_signals;
    else if (sigtype == signal_type::mean)
        return *mean_volatility_10_signals;
    else if (sigtype == signal_type::ssr)
        return *ssr_volatility_10_signals;
    else if (sigtype == signal_type::norm)
        return *norm_volatility_10_signals;
    else
        throw std::runtime_error("Invalid signal type");
}
