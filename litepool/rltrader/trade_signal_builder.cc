#include "trade_signal_builder.h"
#include "rl_macros.h"

using namespace Simulator;

TradeSignalBuilder::TradeSignalBuilder()
                   : raw_signals(std::make_unique<trade_signal_repository>())
{
}


void TradeSignalBuilder::compute_trade_signals(const TradeInfo& info, const double& bid_price, const double& ask_price) {
    trade_signal_repository& repo = *raw_signals;
    auto gross_trade = info.buy_trades + info.sell_trades;
    auto gross_amount = info.buy_amount + info.sell_amount;
    repo.buy_amount_ratio = gross_amount > 0 ? info.buy_amount / gross_amount : 0;
    repo.sell_amount_ratio = gross_amount > 0 ? info.sell_amount / gross_amount: 0;
    repo.relative_buy_price = info.average_buy_price / ask_price;
    repo.relative_sell_price = info.average_sell_price / bid_price;
    repo.buy_num_trade_ratio = gross_trade > 0 ? info.buy_trades / gross_trade : 0;
    repo.sell_num_trade_ratio = gross_trade > 0 ? info.sell_trades / gross_trade : 0;
}

std::vector<double> TradeSignalBuilder::add_trade(const TradeInfo& info, const double& bid_price, const double& ask_price) {
    compute_trade_signals(info, bid_price, ask_price);
    std::vector<double> retval;
    insert_signals(retval, *raw_signals);
    return retval;
}

trade_signal_repository& TradeSignalBuilder::get_trade_signals() const  {
    return *raw_signals;
}