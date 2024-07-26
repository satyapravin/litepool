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
                      ,norm_velocity_10_signals(std::make_unique<position_signal_repository>())
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
    repo.inventory_pnl_drawdown = repo.inventory_pnl_drawdown / info.balance * 10000.0;
    repo.realized_pnl_drawdown = repo.realized_pnl_drawdown / info.balance * 10000.0;
    repo.total_pnl = repo.total_pnl / info.balance * 10000.0;
    repo.total_drawdown = repo.total_drawdown / info.balance * 10000.0;
    raw_signals.add(repo);
    auto& to = *norm_raw_signals;
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
    NORMALIZE(repo, to, mean_velocity_10_signals, ssr_velocity_10_signals, net_position, alpha);
    NORMALIZE(repo, to, mean_velocity_10_signals, ssr_velocity_10_signals, inventory_pnl, alpha);
    NORMALIZE(repo, to, mean_velocity_10_signals, ssr_velocity_10_signals, realized_pnl, alpha);
    NORMALIZE(repo, to, mean_velocity_10_signals, ssr_velocity_10_signals, inventory_pnl_drawdown, alpha);
    NORMALIZE(repo, to, mean_velocity_10_signals, ssr_velocity_10_signals, realized_pnl_drawdown, alpha);
    NORMALIZE(repo, to, mean_velocity_10_signals, ssr_velocity_10_signals, total_pnl, alpha);
    NORMALIZE(repo, to, mean_velocity_10_signals, ssr_velocity_10_signals, total_drawdown, alpha);
    NORMALIZE(repo, to, mean_velocity_10_signals, ssr_velocity_10_signals, relative_price, alpha);
}

void PositionSignalBuilder::compute_volatility() {
    auto& ssr = *ssr_velocity_10_signals;
    position_signal_repository repo;
    repo.inventory_pnl = std::pow(ssr.inventory_pnl, 0.5);
    repo.inventory_pnl_drawdown = std::pow(ssr.inventory_pnl_drawdown, 0.5);
    repo.net_position = std::pow(ssr.net_position, 0.5);
    repo.realized_pnl = std::pow(ssr.realized_pnl, 0.5);
    repo.realized_pnl_drawdown = std::pow(ssr.realized_pnl_drawdown, 0.5);
    repo.relative_price = std::pow(ssr.relative_price, 0.5);
    repo.total_drawdown = std::pow(ssr.total_drawdown, 0.5);
    repo.total_pnl = std::pow(ssr.total_pnl, 0.5);
    
    auto& to = *norm_volatility_10_signals;
    NORMALIZE(repo, to, mean_volatility_10_signals, ssr_volatility_10_signals, net_position, alpha);
    NORMALIZE(repo, to, mean_volatility_10_signals, ssr_volatility_10_signals, inventory_pnl, alpha);
    NORMALIZE(repo, to, mean_volatility_10_signals, ssr_volatility_10_signals, realized_pnl, alpha);
    NORMALIZE(repo, to, mean_volatility_10_signals, ssr_volatility_10_signals, inventory_pnl_drawdown, alpha);
    NORMALIZE(repo, to, mean_volatility_10_signals, ssr_volatility_10_signals, realized_pnl_drawdown, alpha);
    NORMALIZE(repo, to, mean_volatility_10_signals, ssr_volatility_10_signals, total_pnl, alpha);
    NORMALIZE(repo, to, mean_volatility_10_signals, ssr_volatility_10_signals, total_drawdown, alpha);
    NORMALIZE(repo, to, mean_volatility_10_signals, ssr_volatility_10_signals, relative_price, alpha);
}

position_signal_repository& PositionSignalBuilder::get_position_signals(signal_type sigtype) {
    if (sigtype == signal_type::raw)
        return raw_signals.get(0);
    else if (sigtype == signal_type::mean)
        return *mean_raw_signals;
    else if (sigtype == signal_type::ssr)
        return *ssr_raw_signals;
    else
        return *norm_raw_signals;
}

position_signal_repository& PositionSignalBuilder::get_velocity_signals(signal_type sigtype) {
    if (sigtype == signal_type::raw)
        return *velocity_10_signals;
    else if (sigtype == signal_type::mean)
        return *mean_velocity_10_signals;
    else if (sigtype == signal_type::ssr)
        return *ssr_velocity_10_signals;
    else
        return *norm_velocity_10_signals;
}

position_signal_repository& PositionSignalBuilder::get_volatility_signals(signal_type sigtype) {
    if (sigtype == signal_type::raw)
        return *ssr_velocity_10_signals;
    else if (sigtype == signal_type::mean)
        return *mean_volatility_10_signals;
    else if (sigtype == signal_type::ssr)
        return *ssr_volatility_10_signals;
    else
        return *norm_volatility_10_signals;
}


