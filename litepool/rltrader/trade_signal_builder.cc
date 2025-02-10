#include "trade_signal_builder.h"
#include "rl_macros.h"

using namespace RLTrader;

TradeSignalBuilder::TradeSignalBuilder()
                   : raw_previous_signals(std::make_unique<trade_signal_repository>()),
                     raw_spread_signals(std::make_unique<trade_signal_repository>())
{
}


void TradeSignalBuilder::compute_trade_signals(const TradeInfo& info, const double& bid_price, const double& ask_price) {
    trade_signal_repository repo;
    trade_signal_repository& previous_repo = *raw_previous_signals;
    auto gross_trade = static_cast<double>(info.buy_trades + info.sell_trades);
    double gross_amount = info.buy_amount + info.sell_amount;
    repo.buy_amount_ratio = gross_amount > 0 ? info.buy_amount / gross_amount : 0;
    repo.sell_amount_ratio = gross_amount > 0 ? info.sell_amount / gross_amount: 0;
    repo.relative_buy_price = info.average_buy_price / ask_price;
    repo.relative_sell_price = info.average_sell_price / bid_price;
    repo.buy_num_trade_ratio = gross_trade > 0 ? info.buy_trades * / gross_trade : 0.0;
    repo.sell_num_trade_ratio = gross_trade > 0 ? info.sell_trades / gross_trade : 0.0;

    raw_spread_signals->buy_amount_ratio = repo.buy_amount_ratio - previous_repo.buy_amount_ratio;
    raw_spread_signals->buy_num_trade_ratio = repo.buy_num_trade_ratio - previous_repo.buy_num_trade_ratio;
    raw_spread_signals->relative_buy_price = repo.relative_buy_price - previous_repo.relative_buy_price;
    raw_spread_signals->relative_sell_price = repo.relative_sell_price - previous_repo.relative_sell_price;
    raw_spread_signals->sell_amount_ratio = repo.sell_amount_ratio - previous_repo.sell_amount_ratio;
    raw_spread_signals->sell_num_trade_ratio = repo.sell_num_trade_ratio - previous_repo.sell_num_trade_ratio;

    raw_previous_signals->buy_amount_ratio = repo.buy_amount_ratio;
    raw_previous_signals->buy_num_trade_ratio =repo.buy_num_trade_ratio;
    raw_previous_signals->relative_buy_price = repo.relative_buy_price;
    raw_previous_signals->relative_sell_price = repo.relative_sell_price;
    raw_previous_signals->sell_amount_ratio = repo.sell_amount_ratio;
    raw_previous_signals->sell_num_trade_ratio = repo.sell_num_trade_ratio;
}

std::vector<double> TradeSignalBuilder::add_trade(const TradeInfo& info, const double& bid_price, const double& ask_price) {
    compute_trade_signals(info, bid_price, ask_price);
    std::vector<double> retval;
    insert_signals(retval, *raw_spread_signals);
    return retval;
}
