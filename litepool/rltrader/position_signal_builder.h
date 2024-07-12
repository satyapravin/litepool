#pragma once

#include <deque>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <memory>
#include "position.h"
#include "circ_buffer.h"
#include "circ_table.h"

struct position_signal_repository {
    double net_position = 0;
    double relative_price = 0;
    double total_trade_amount = 0;
    double buy_num_trade_ratio = 0;
    double sell_num_trade_ratio = 0;
    double number_of_trades = 0;
    double average_trade_amount = 0;
    double buy_amount_ratio = 0;
    double sell_amount_ratio = 0;
    double duration_since_trade = 0;
    double duration_between_fills = 0;
    double amount_per_duration = 0;
    double steps_until_episode = 0;
    double steps_since_episode = 0;
    double pnl = 0;
    double drawdown = 0;
    double realized_pnl = 0;
    double inventory_pnl = 0;
};

class PositionSignalBuilder {

};