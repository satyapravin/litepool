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

#include "litepool/core/async_litepool.h"
#include "litepool/core/env.h"
#include "env_adaptor.h"
#include "normal_instrument.h"
#include <random>

namespace rltrader {

class RlTraderEnvFns {
 public:
  static decltype(auto) DefaultConfig() {
    return MakeDict("filename"_.Bind(std::string("")), "balance"_.Bind(1.0), "depth"_.Bind<int>(5));
  }

  template <typename Config>
  static decltype(auto) StateSpec(const Config& conf) {
    float fmax = std::numeric_limits<float>::max();

    return MakeDict("obs"_.Bind(Spec<float>({62}, {-fmax, fmax})),
                    "info:mid_price"_.Bind(Spec<float>({})),
                    "info:balance"_.Bind(Spec<float>({})),
                    "info:unrealized_pnl"_.Bind(Spec<float>({})),
                    "info:realized_pnl"_.Bind(Spec<float>({})),
                    "info:leverage"_.Bind(Spec<float>({})),
                    "info:trade_count"_.Bind(Spec<float>({})),
                    "info:inventory_drawdown"_.Bind(Spec<float>({})),
                    "info:drawdown"_.Bind(Spec<float>({})),
                    "info:fees"_.Bind((Spec<float>({}))),
                    "info:buy_amount"_.Bind((Spec<float>({}))),
                    "info:sell_amount"_.Bind((Spec<float>({}))));
  }

  template <typename Config>
  static decltype(auto) ActionSpec(const Config& conf) {
    std::vector<int> shape = {1};
    int spread_min = 0;
    int spread_max = 15;
    int vol_min = 1;
    int vol_max = 10;
    return MakeDict("action"_.Bind(Spec<int>({4}, {{spread_min, spread_min, vol_min, vol_min},
                                                      {spread_max, spread_max, vol_max, vol_max}})));
  }
};

using RlTraderEnvSpec = EnvSpec<RlTraderEnvFns>;

class RlTraderEnv : public Env<RlTraderEnvSpec> {
 protected:
  int state_{0};
  long long timestamp = 0;
  bool isDone = true;
  std::string filename;
  double balance = 0;
  long long steps = 0;
  double previous_upnl = 0;
  double previous_rpnl = 0;
  double holding_period = 0;
  double previous_fees = 0;
  double previous_leverage = 0;
  std::unique_ptr<Simulator::NormalInstrument> instr_ptr;
  std::unique_ptr<Simulator::Exchange> exchange_ptr;
  std::unique_ptr<Simulator::Strategy> strategy_ptr;
  std::unique_ptr<Simulator::EnvAdaptor> adaptor_ptr;

 public:
  RlTraderEnv(const Spec& spec, int env_id) : Env<RlTraderEnvSpec>(spec, env_id),
                                              filename(spec.config["filename"_]),
                                              balance(spec.config["balance"_])
  {
    seed_ = env_id + 1;
    if (seed_ < 1) {
      seed_ = 1;Simulator::CsvReader reader(filename);

    }

    instr_ptr = std::make_unique<Simulator::NormalInstrument>("BTCUSDT", 0.1,
                                                                0.0001, -0.0001, 0.0075);
    exchange_ptr = std::make_unique<Simulator::Exchange>(filename, 250);
    strategy_ptr = std::make_unique<Simulator::Strategy>(*instr_ptr, *exchange_ptr, balance, 0, 0, 20);
    adaptor_ptr = std::make_unique<Simulator::EnvAdaptor>(*strategy_ptr, *exchange_ptr, spec.config["depth"_]);
  }

  void Reset() override {
    std::mt19937 rng;
    std::random_device rd;
    steps = 0;
    previous_upnl = 0;
    previous_rpnl = 0;
    holding_period = 0;
    previous_fees = 0;
    previous_leverage = 0;
    rng.seed(rd());
    std::uniform_int_distribution<int> dist(10, 72);
    adaptor_ptr->reset(dist(rng), 0, 0);
    timestamp = adaptor_ptr->getTime();
    isDone = false;
    WriteState();
  }

  void Step(const Action& action) override {
      auto buy_spread = action["action"_][0];
      auto sell_spread = action["action"_][1];
      auto buy_volume = action["action"_][2];
      auto sell_volume = action["action"_][3];

      adaptor_ptr->quote(buy_spread, sell_spread, buy_volume, sell_volume);
      auto info = adaptor_ptr->getInfo();
      isDone = !adaptor_ptr->next() || info["leverage"] > 25;
      ++steps;
      WriteState();
  }

  void WriteState() {
    auto data = adaptor_ptr->getState();
    State state = Allocate(1);

    if (!isDone) {
      assert(data.size() == 62);
    }

    auto info = adaptor_ptr->getInfo();
    state["info:mid_price"_] = static_cast<float>(info["mid_price"]);
    state["info:balance"_] = static_cast<float>(info["balance"]);
    state["info:unrealized_pnl"_] = static_cast<float>(info["unrealized_pnl"]);
    state["info:realized_pnl"_] = static_cast<float>(info["realized_pnl"]);
    state["info:leverage"_] = static_cast<float>(info["leverage"]);
    state["info:trade_count"_] = static_cast<float>(info["trade_count"]);
    state["info:drawdown"_] = static_cast<float>(info["drawdown"]);
    state["info:fees"_] = static_cast<float>(info["fees"]);
    state["info:buy_amount"_] = static_cast<float>(info["buy_amount"]);
    state["info:sell_amount"_] = static_cast<float>(info["sell_amount"]);
    double drawdown = info["drawdown"];
    double upnl = info["unrealized_pnl"];
    double rpnl = info["realized_pnl"];
    double leverage = info["buy_amount"] - info["sell_amount"];
    double init_balance = 0.02;

    if (isDone || steps % 200 == 0)  {
      auto netpnl = (rpnl + upnl) / init_balance;

      if (netpnl > 0) {
        state["reward"_] =  netpnl / std::abs(drawdown);
      }
      else {
        state["reward"_] = netpnl + drawdown;
      }

      if (isDone) return;
    }
    else {
      auto abs_leverage = std::abs(leverage);

      if (std::signbit(leverage) == std::signbit(previous_leverage)) {
          holding_period += 1;
      } else {
          holding_period = 0;
      }
      state["reward"_] = -0.001 * holding_period +
                         std::signbit(holding_period) * (leverage - previous_leverage) +
                         (rpnl - previous_rpnl) / init_balance +
                         std::min(0.0, upnl - previous_upnl) / init_balance;
      previous_upnl = upnl;
      previous_rpnl = rpnl;
      previous_fees = info["fees"];
      previous_leverage = leverage;
    }

    for(int ii=0; ii < data.size(); ++ii) {
      state["obs"_](ii) = static_cast<float>(data[ii]);
    }
    steps++;
  }

  bool IsDone() override { return isDone; }
};

using RlTraderLitePool = AsyncLitePool<RlTraderEnv>;

}  // namespace rltrader

#endif  // LITEPOOL_RLTRADER_RLTRADER_LITEPOOL_H_
