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

#include <iostream>
#include <memory>

#include "litepool/core/async_litepool.h"
#include "litepool/core/env.h"
#include "env_adaptor.h"
#include "inverse_instrument.h"
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

    return MakeDict("obs"_.Bind(Spec<float>({258}, {-fmax, fmax})),
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
    return MakeDict("action"_.Bind(Spec<float>({2}, {2, 88})));
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
  double drawdown = 0;
  double previous_pnl = 0;
  double previous_dd = 0;
  std::unique_ptr<Simulator::CsvReader> reader_ptr;
  std::unique_ptr<Simulator::InverseInstrument> instr_ptr;
  std::unique_ptr<Simulator::Exchange> exchange_ptr;
  std::unique_ptr<Simulator::Strategy> strategy_ptr;
  std::unique_ptr<Simulator::EnvAdaptor> adaptor_ptr;

 public:
  RlTraderEnv(const Spec& spec, int env_id) : Env<RlTraderEnvSpec>(spec, env_id),
                                              filename(spec.config["filename"_]),
                                              balance(spec.config["balance"_])
  {
    if (seed_ < 1) {
      seed_ = 1;Simulator::CsvReader reader(filename);

    }

    reader_ptr = std::make_unique<Simulator::CsvReader>(filename);
    instr_ptr = std::make_unique<Simulator::InverseInstrument>("BTCUSD", 0.5,
                                                                10, 0.0001, 0);
    exchange_ptr = std::make_unique<Simulator::Exchange>(*reader_ptr, 300);
    strategy_ptr = std::make_unique<Simulator::Strategy>(*instr_ptr, *exchange_ptr, balance, 0, 0, 30, 20);
    adaptor_ptr = std::make_unique<Simulator::EnvAdaptor>(*strategy_ptr, *exchange_ptr, 20, 20, spec.config["depth"_]);
  }

  void Reset() override {
    std::mt19937 rng;
    std::random_device rd;
    steps = 0;
    drawdown = 0;
    previous_pnl = 0;
    previous_dd = 0;
    rng.seed(rd());
    std::uniform_int_distribution<int> dist(10, 72);
    adaptor_ptr->reset(dist(rng), 0, 0);

    while(!adaptor_ptr->is_data_ready()) {
        adaptor_ptr->next();
    }

    timestamp = adaptor_ptr->getTime();
    isDone = false;
    WriteState();
  }

  void Step(const Action& action) override {
      auto buy_angle = action["action"_][0];
      auto sell_angle = action["action"_][1];
      adaptor_ptr->quote(buy_angle, sell_angle);
      isDone = !adaptor_ptr->next();
      ++steps;
      WriteState();
  }

  void WriteState() {
    auto data = adaptor_ptr->getState();
    State state = Allocate(1);

    if (!isDone)
        assert(data.size() == 258);

    for(int ii=0; ii < data.size(); ++ii) {
      state["obs"_](ii) = static_cast<float>(data[ii]);
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
    drawdown = info["drawdown"];
    double pnl = static_cast<float>(std::min(info["unrealized_pnl"], 0.0)) +
                         static_cast<float>(info["realized_pnl"]);
    if (isDone) {
      state["reward"_] = pnl + drawdown - info["leverage"];
    }
    else if (steps % 120 == 0) {
      state["reward"_] = pnl + drawdown -previous_dd - previous_pnl - 10.0 * info["leverage"];

    } else {
      state["reward"_] = 0;
    }

    previous_dd = drawdown;
    previous_pnl = pnl;
  }

  bool IsDone() override { return isDone; }
};

using RlTraderLitePool = AsyncLitePool<RlTraderEnv>;

}  // namespace rltrader

#endif  // LITEPOOL_RLTRADER_RLTRADER_LITEPOOL_H_
