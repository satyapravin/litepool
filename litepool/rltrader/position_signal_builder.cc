#include "position_signal_builder.h"
#include "rl_macros.h"

using namespace Simulator;

PositionSignalBuilder::PositionSignalBuilder()
                      :raw_signals(std::make_unique<position_signal_repository>()) {
}

std::vector<double> PositionSignalBuilder::add_info(const PositionInfo& info, const double& bid_price, const double& ask_price) {
    compute_signals(info, bid_price, ask_price);

    std::vector<double> retval;
    insert_signals(retval, *raw_signals);
    return retval;
}

void PositionSignalBuilder::compute_signals(const PositionInfo& info, const double& bid_price, const double& ask_price) {
    position_signal_repository& repo = *raw_signals;
    repo.net_position = info.netPosition;
    repo.inventory_pnl = info.inventoryPnL;
    repo.realized_pnl = info.tradingPnL;

    if (max_inventory_pnl < repo.inventory_pnl) max_inventory_pnl = repo.inventory_pnl;
    if (max_trading_pnl < repo.realized_pnl) max_trading_pnl = repo.realized_pnl;
    repo.inventory_pnl_drawdown = std::min(repo.inventory_pnl - max_inventory_pnl, 0.0);
    repo.realized_pnl_drawdown = std::min(repo.realized_pnl - max_trading_pnl, 0.0);
    repo.total_pnl = repo.inventory_pnl + repo.realized_pnl;
    repo.total_drawdown = repo.inventory_pnl_drawdown + repo.realized_pnl_drawdown;
    repo.relative_price = info.averagePrice / (info.netPosition > 0 ? bid_price : ask_price);

    repo.net_position = repo.net_position / info.balance;
    repo.inventory_pnl = repo.inventory_pnl / info.balance;
    repo.realized_pnl = repo.realized_pnl / info.balance;
    repo.inventory_pnl_drawdown = repo.inventory_pnl_drawdown / info.balance;
    repo.realized_pnl_drawdown = repo.realized_pnl_drawdown / info.balance;
    repo.total_pnl = repo.total_pnl / info.balance;
    repo.total_drawdown = repo.total_drawdown / info.balance;
}

position_signal_repository& PositionSignalBuilder::get_position_signals() const {
    return *raw_signals;
}

