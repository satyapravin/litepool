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
#include <cmath>
#include <algorithm>
#include <numeric>
#include <iostream>
#include "litepool/core/async_litepool.h"
#include "litepool/core/env.h"
#include "env_adaptor.h"
#include "normal_instrument.h"
#include <random>
#include <filesystem>

namespace fs = std::filesystem;
namespace rltrader {

class RlTraderEnvFns {
 public:
  static decltype(auto) DefaultConfig() {
    return MakeDict("foldername"_.Bind(std::string("")), "balance"_.Bind(1.0), "depth"_.Bind<int>(5), "start"_.Bind<int>(0), "max"_.Bind<int>(72000));
  }

  template <typename Config>
  static decltype(auto) StateSpec(const Config& conf) {
    return MakeDict("obs"_.Bind(Spec<float>({490})),
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
    return MakeDict("action"_.Bind(Spec<float>({8}, {{-1., -1., -1., -1., -1., -1., -1., -1.},
                                                     { 1.,  1.,  1.,  1.,  1.,  1.,  1.,  1.}})));
  }
};


using RlTraderEnvSpec = EnvSpec<RlTraderEnvFns>;

class RlTraderEnv : public Env<RlTraderEnvSpec> {
 protected:
  double spreads[4] = {0, 2, 4, 10};
  int state_{0};
  bool isDone = true;
  std::string foldername;
  double balance = 0;
  int start_read = 0;
  int max_read = 0;
  long long steps = 0;
  double previous_rpnl = 0;
  double previous_upnl = 0;
  double previous_fees = 0;
  std::unique_ptr<Simulator::NormalInstrument> instr_ptr;
  std::unique_ptr<Simulator::SimExchange> exchange_ptr;
  std::unique_ptr<Simulator::Strategy> strategy_ptr;
  std::unique_ptr<Simulator::EnvAdaptor> adaptor_ptr;
 public:
  RlTraderEnv(const Spec& spec, int env_id) : Env<RlTraderEnvSpec>(spec, env_id),
                                              foldername(spec.config["foldername"_]),
                                              balance(spec.config["balance"_]),
                                              start_read(spec.config["start"_]),
                                              max_read(spec.config["max"_])
  {
    instr_ptr = std::make_unique<Simulator::NormalInstrument>("BTCUSDT", 0.5,
                                                                0.0002, -0.0001, 0.0005);
    int idx = env_id % 45;
    std::string filename = foldername + std::to_string(idx + 1) + ".csv";
    std::cout << filename << std::endl;
    exchange_ptr = std::make_unique<Simulator::SimExchange>(filename, 250, start_read, max_read);
    strategy_ptr = std::make_unique<Simulator::Strategy>(*instr_ptr, *exchange_ptr, balance, 0, 0, 20);
    adaptor_ptr = std::make_unique<Simulator::EnvAdaptor>(*strategy_ptr, *exchange_ptr, spec.config["depth"_]);
  }

  void Reset() override {
    steps = 0;
    previous_rpnl = 0;
    previous_upnl = 0;
    previous_fees = 0;
    adaptor_ptr->reset(0, 0);
    isDone = false;
    WriteState();
  }

  int select_action(const std::vector<double>& logits) {
      auto maxLogitIt = std::max_element(logits.begin(), logits.end());
      return std::distance(logits.begin(), maxLogitIt);
  }


  void Step(const Action& action) override {
      std::vector<double> buyActionLogits { action["action"_][0],
                                            action["action"_][1],
                                            action["action"_][2],
                                            action["action"_][3],
                                          };

      std::vector<double> sellActionLogits { action["action"_][4],
                                             action["action"_][5],
                                             action["action"_][6],
                                             action["action"_][7],
                                           };

      auto buy_action = select_action(buyActionLogits);
      auto sell_action = select_action(sellActionLogits);
      auto buy_spread = spreads[buy_action];
      auto sell_spread = spreads[sell_action];
      adaptor_ptr->quote(buy_spread, sell_spread, 5, 5);
      auto info = adaptor_ptr->getInfo();
      isDone = !adaptor_ptr->next();
      ++steps;
      WriteState();
  }

  void WriteState() {
    auto data = adaptor_ptr->getState();
    State state = Allocate(1);

    if (!isDone) {
      assert(data.size() == 490);
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

    auto pnl = info["realized_pnl"] - previous_rpnl; 
    auto upnl = info["unrealized_pnl"]- previous_upnl;
    state["reward"_] = (previous_fees - info["fees"]) + pnl + upnl; 

    if (isDone) return;
    previous_rpnl = info["realized_pnl"];
    previous_upnl = info["unrealized_pnl"];
    previous_fees = info["fees"];
       
    for(int ii=0; ii < data.size(); ++ii) {
      state["obs"_](ii) = static_cast<float>(data[ii]);
    }
  }

  bool IsDone() override { return isDone; }
};

using RlTraderLitePool = AsyncLitePool<RlTraderEnv>;

}  // namespace rltrader

#endif  // LITEPOOL_RLTRADER_RLTRADER_LITEPOOL_H_
