#include "position_signal_builder.h"
#include "norm_macro.h"

using namespace Simulator;

PositionSignalBuilder::PositionSignalBuilder()
                      :raw_signals(10)
                      ,mean_raw_signals(std::make_unique<position_signal_repository>())
                      ,ssr_raw_signals(std::make_unique<position_signal_repository>())
                      ,norm_raw_signals(std::make_unique<position_signal_repository>())
                      ,velocity_10_signals(std::make_unique<position_signal_repository>())
                      ,mean_velocity_10_signals(std::make_unique<position_signal_repository>())
                      ,ssr_velocity_10_signals(std::make_unique<position_signal_repository>())
                      ,volatility_10_signals(std::make_unique<position_signal_repository>())
                      ,mean_volatility_10_signals(std::make_unique<position_signal_repository>())
                      ,ssr_volatility_10_signals(std::make_unique<position_signal_repository>())
                      ,norm_volatility_10_signals(std::make_unique<position_signal_repository>()) {
}

std::vector<double> PositionSignalBuilder::add_info(PositionInfo& info, double& bid_price, double& ask_price) {
    compute_signals(info, bid_price, ask_price);

    if (processed_counter > 10) {
        compute_velocity();
        compute_volatility();
    }

    std::vector<double> retval;
    insert_signals(retval, *norm_raw_signals);
    insert_signals(retval, *norm_velocity_10_signals);
    insert_signals(retval, *norm_volatility_10_signals);
    processed_counter++;
    return retval;
}

void PositionSignalBuilder::compute_signals(PositionInfo& info, double& bid_price, double& ask_price) {
    position_signal_repository repo;
    repo.net_position = info.netPosition;
    repo.inventory_pnl = info.inventoryPnL;
    repo.realized_pnl = info.tradingPnL;
    repo.inventory_pnl_drawdown = 0;
    repo.realized_pnl_drawdown = 0;
    repo.total_pnl = repo.inventory_pnl + repo.realized_pnl;
    repo.total_drawdown = repo.inventory_pnl_drawdown + repo.realized_pnl_drawdown;
    repo.relative_price = info.averagePrice / (info.netPosition > 0 ? bid_price : ask_price);
    raw_signals.add(repo);
    auto& to = raw_signals.get(0);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, net_position, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, inventory_pnl, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, realized_pnl, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, inventory_pnl_drawdown, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, realized_pnl_drawdown, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, total_pnl, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, total_drawdown, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, relative_price, alpha);
}

void PositionSignalBuilder::compute_velocity() {
    auto& current = raw_signals.get(0);
    auto& prev10 = raw_signals.get(10);
    position_signal_repository& repo = *velocity_10_signals;
    repo.net_position = current.net_position - prev10.net_position;
    repo.inventory_pnl = current.inventory_pnl - prev10.inventory_pnl;
    repo.realized_pnl = current.realized_pnl - prev10.realized_pnl;
    repo.inventory_pnl_drawdown = current.inventory_pnl_drawdown - prev10.inventory_pnl_drawdown;
    repo.realized_pnl_drawdown = current.realized_pnl_drawdown - prev10.realized_pnl_drawdown;
    repo.total_pnl = current.total_pnl - prev10.total_pnl;
    repo.total_drawdown = current.total_drawdown - prev10.total_drawdown;
    repo.relative_price = current.relative_price - prev10.relative_price;
    auto& to = *norm_velocity_10_signals;
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, net_position, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, inventory_pnl, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, realized_pnl, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, inventory_pnl_drawdown, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, realized_pnl_drawdown, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, total_pnl, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, total_drawdown, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, relative_price, alpha);
}

void PositionSignalBuilder::compute_volatility() {
    auto& repo = *ssr_velocity_10_signals;
    auto& to = *norm_volatility_10_signals;
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, net_position, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, inventory_pnl, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, realized_pnl, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, inventory_pnl_drawdown, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, realized_pnl_drawdown, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, total_pnl, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, total_drawdown, alpha);
    NORMALIZE(repo, to, mean_raw_signals, ssr_raw_signals, relative_price, alpha);
}

position_signal_repository& PositionSignalBuilder::get_position_signals(signal_type sigtype) {

}

position_signal_repository& PositionSignalBuilder::get_velocity_signals(signal_type sigtype) {

}

position_signal_repository& PositionSignalBuilder::get_volatility_signals(signal_type sigtype) {

}


