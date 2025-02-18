// Copyright 2021 Garena Online Private Limited
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef LITEPOOL_RLTRADER_RLTRADER_LITEPOOL_H_
#define LITEPOOL_RLTRADER_RLTRADER_LITEPOOL_H_

#include <memory>
#include <vector>
#include <algorithm>
#include <iostream>
#include <tuple>
#include "litepool/core/async_litepool.h"
#include "litepool/core/env.h"
#include "env_adaptor.h"

#include "base_instrument.h"
#include "inverse_instrument.h"
#include "normal_instrument.h"

#include <filesystem>

#include "deribit_exchange.h"
#include "sim_exchange.h"

namespace fs = std::filesystem;
namespace rltrader {

class RlTraderEnvFns {
 public:
  static decltype(auto) DefaultConfig() {
    return MakeDict("is_prod"_.Bind<bool>(false),
                    "api_key"_.Bind(std::string("")),
                    "api_secret"_.Bind(std::string("")),
                    "is_inverse_instr"_.Bind<bool>(true),
                    "symbol"_.Bind((std::string(""))),
                    "tick_size"_.Bind<double>(0.5),
                    "min_amount"_.Bind<double>(10.0),
                    "maker_fee"_.Bind<double>(-0.0001),
                    "taker_fee"_.Bind<double>(0.0005),
                    "foldername"_.Bind(std::string("./train_files/")),
                    "balance"_.Bind(1.0),
                    "start"_.Bind<int>(0),
                    "max"_.Bind<int>(72000));
  }

  template <typename Config>
  static decltype(auto) StateSpec(const Config& conf) {
    return MakeDict("obs"_.Bind(Spec<double>({196})),
                    "info:mid_price"_.Bind(Spec<double>({-1})),
                    "info:balance"_.Bind(Spec<double>({-1})),
                    "info:unrealized_pnl"_.Bind(Spec<double>({-1})),
                    "info:realized_pnl"_.Bind(Spec<double>({-1})),
                    "info:leverage"_.Bind(Spec<double>({-1})),
                    "info:trade_count"_.Bind(Spec<double>({-1})),
                    "info:inventory_drawdown"_.Bind(Spec<double>({-1})),
                    "info:drawdown"_.Bind(Spec<double>({-1})),
                    "info:fees"_.Bind((Spec<double>({-1}))),
                    "info:buy_amount"_.Bind((Spec<double>({-1}))),
                    "info:sell_amount"_.Bind((Spec<double>({-1}))));
  }

  template <typename Config>
  static decltype(auto) ActionSpec(const Config& conf) {
    std::tuple<int, int> bounds = {0, 15};
    return MakeDict("action"_.Bind(Spec<int>({-1}, bounds)));
  }
};


using RlTraderEnvSpec = EnvSpec<RlTraderEnvFns>;

class RlTraderEnv : public Env<RlTraderEnvSpec> {
 protected:
  int spreads[4] = {0, 2, 4, 10};
  int state_{0};
  bool isDone = true;
  bool is_prod = false;
  bool is_inverse_instr = false;
  std::string api_key;
  std::string api_secret;
  std::string symbol;
  double tick_size;
  double min_amount;
  double maker_fee;
  double taker_fee;
  std::string foldername;
  double balance = 0;
  int start_read = 0;
  int max_read = 0;
  long long steps = 0;
  double previous_buy_sell_diff = 0;
  double previous_rpnl = 0;
  double previous_upnl = 0;
  double previous_fees = 0;
  std::unique_ptr<RLTrader::BaseInstrument> instr_ptr;
  std::unique_ptr<RLTrader::BaseExchange> exchange_ptr;
  std::unique_ptr<RLTrader::Strategy> strategy_ptr;
  std::unique_ptr<RLTrader::EnvAdaptor> adaptor_ptr;
 public:
  RlTraderEnv(const Spec& spec, int env_id) : Env<RlTraderEnvSpec>(spec, env_id),
                                              is_prod(spec.config["is_prod"_]),
                                              is_inverse_instr(spec.config["is_inverse_instr"_]),
                                              api_key(spec.config["api_key"_]),
                                              api_secret(spec.config["api_secret"_]),
                                              symbol(spec.config["symbol"_]),
                                              tick_size(spec.config["tick_size"_]),
                                              min_amount(spec.config["min_amount"_]),
                                              maker_fee(spec.config["maker_fee"_]),
                                              taker_fee(spec.config["taker_fee"_]),
                                              foldername(spec.config["foldername"_]),
                                              balance(spec.config["balance"_]),
                                              start_read(spec.config["start"_]),
                                              max_read(spec.config["max"_])
  {

    RLTrader::BaseInstrument* instr_raw_ptr = nullptr;
    RLTrader::BaseExchange* exch_raw_ptr = nullptr;


    if (this->is_inverse_instr) {
      instr_raw_ptr = new RLTrader::InverseInstrument(symbol, tick_size, min_amount, maker_fee, taker_fee);
    } else {
      instr_raw_ptr = new RLTrader::NormalInstrument(symbol, tick_size, min_amount, maker_fee, taker_fee);
    }


    if (this->is_prod) {
      exch_raw_ptr = new RLTrader::DeribitExchange(symbol, api_key, api_secret);
    } else {
      int idx = env_id % 64;
      std::string filename = foldername + std::to_string(idx + 1) + ".csv";
      std::cout << filename << std::endl;
      exch_raw_ptr = new RLTrader::SimExchange(filename, 250, start_read, max_read);
    }

    instr_ptr.reset(instr_raw_ptr);
    exchange_ptr.reset(exch_raw_ptr);
    strategy_ptr = std::make_unique<RLTrader::Strategy>(*instr_ptr, *exchange_ptr, balance, 20);
    adaptor_ptr = std::make_unique<RLTrader::EnvAdaptor>(*strategy_ptr, *exchange_ptr);
  }

  void Reset() override {
    steps = 0;
    previous_buy_sell_diff = 0;
    previous_rpnl = 0;
    previous_upnl = 0;
    previous_fees = 0;
    adaptor_ptr->reset();
    isDone = false;
    WriteState();
  }

  void Step(const Action& action_dict) override {
      auto action = static_cast<int>(action_dict["action"_][0]);
      int buy_action = static_cast<int>(action / 4);
      auto sell_action = action % 4;
      auto buy_spread = spreads[buy_action];
      auto sell_spread = spreads[sell_action];
      int base_vol = 2;
      adaptor_ptr->quote(buy_spread, sell_spread, base_vol, base_vol);
      isDone = !adaptor_ptr->next();
      ++steps;
      WriteState();
  }

  void WriteState() {
    std::array<double, 196> data;
    adaptor_ptr->getState(data);
    State state = Allocate(1);

    if (!isDone) {
      assert(data.size() == 98*2);
    }
    
    std::unordered_map<std::string, double> info;
    adaptor_ptr->getInfo(info);
    state["info:mid_price"_] = info["mid_price"];
    state["info:balance"_] = info["balance"];
    state["info:unrealized_pnl"_] = info["unrealized_pnl"];
    state["info:realized_pnl"_] = info["realized_pnl"];
    state["info:leverage"_] = info["leverage"];
    state["info:trade_count"_] = info["trade_count"];
    state["info:drawdown"_] = info["drawdown"];
    state["info:fees"_] = info["fees"];


    auto pnl = info["realized_pnl"] - previous_rpnl; 
    auto upnl = info["unrealized_pnl"]- previous_upnl;
    state["reward"_] = (previous_fees - info["fees"]); + pnl + upnl;

    auto buy_sell_diff = 0.0;

    if (info["leverage"] > 0) {
	buy_sell_diff = (info["curr_price"] - info["avg_buy_price"]) / info["curr_price"];
    } else if (info["leverage"] < 0) {
	buy_sell_diff = (info["avg_sell_price"] - info["curr_price"]) / info["curr_price"];
    } else { buy_sell_diff = 0; }

    state["reward"_] += buy_sell_diff - previous_buy_sell_diff;
    previous_buy_sell_diff = buy_sell_diff;
    previous_rpnl = info["realized_pnl"];
    previous_upnl = info["unrealized_pnl"];
    previous_fees = info["fees"];
    state["obs"_].Assign(data.begin(), data.size());
    if (isDone) return;
  }

  bool IsDone() override { return isDone; }
};

using RlTraderLitePool = AsyncLitePool<RlTraderEnv>;

}  // namespace rltrader

#endif  // LITEPOOL_RLTRADER_RLTRADER_LITEPOOL_H_
