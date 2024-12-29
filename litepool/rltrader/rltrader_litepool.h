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
    return MakeDict("obs"_.Bind(Spec<float>({980*2})),
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
    return MakeDict("action"_.Bind(Spec<float>({4}, {{0., 0., 0.1, 0.1},{1., 1., 1., 1.}})));
  }
};


class ContinuousToSoftDiscrete {
public:
    ContinuousToSoftDiscrete(int n_discrete, double low, double high)
        : n_discrete(n_discrete), low(low), high(high) {
        generateDiscreteActions();
    }

    int mapToDiscrete(double mean, double std) {
        std::vector<double> logits(n_discrete);
        for (int i = 0; i < n_discrete; ++i) {
            double x = discrete_actions[i];
            logits[i] = computeLogit(x, mean, std);
        }

        std::vector<double> probs = softmax(logits);
        return std::distance(probs.begin(), std::max_element(probs.begin(), probs.end()));
    }

private:
    int n_discrete;
    double low, high;
    std::vector<double> discrete_actions;

    void generateDiscreteActions() {
        discrete_actions.resize(n_discrete);
        double step = (high - low) / (n_discrete - 1);
        for (int i = 0; i < n_discrete; ++i) {
            discrete_actions[i] = low + i * step;
        }
    }

    double computeLogit(double x, double mean, double std) {
        double diff = x - mean;
        return -((diff * diff) / (2.0 * std * std));
    }

    std::vector<double> softmax(const std::vector<double>& logits) {
        std::vector<double> probs(logits.size());
        double max_logit = *std::max_element(logits.begin(), logits.end());
        double sum_exp = 0.0;
        for (size_t i = 0; i < logits.size(); ++i) {
            probs[i] = std::exp(logits[i] - max_logit);
            sum_exp += probs[i];
        }

        for (size_t i = 0; i < probs.size(); ++i) {
            probs[i] /= sum_exp;
        }

        return probs;
    }
};


class RollingSD {
private:
    int count;      
    double mean;    
    double m2;      

public:
    RollingSD() : count(0), mean(0.0), m2(0.0) {}

    void add(double value) {
        count++;
        double delta = value - mean;  
        mean += delta / count;       
        double delta2 = value - mean;  
        m2 += delta * delta2;        
    }

    double avg() const {
        return mean;
    }

    double sdev() const {
        if (count < 2) {
            return 0.0;
        }
        return std::sqrt(m2 / (count - 1)); 
    }


    void reset() {
        count = 0;
        mean = 0.0;
        m2 = 0.0;
    }
};

using RlTraderEnvSpec = EnvSpec<RlTraderEnvFns>;

class RlTraderEnv : public Env<RlTraderEnvSpec> {
 protected:
  int state_{0};
  long long timestamp = 0;
  bool isDone = true;
  std::string foldername;
  double balance = 0;
  int start_read = 0;
  int max_read = 0;
  long long steps = 0;
  double previous_rpnl = 0;
  double previous_upnl = 0;
  double previous_fees = 0;
  ContinuousToSoftDiscrete bidder;
  ContinuousToSoftDiscrete asker;
  std::unique_ptr<Simulator::NormalInstrument> instr_ptr;
  std::unique_ptr<Simulator::Exchange> exchange_ptr;
  std::unique_ptr<Simulator::Strategy> strategy_ptr;
  std::unique_ptr<Simulator::EnvAdaptor> adaptor_ptr;
 public:
  RlTraderEnv(const Spec& spec, int env_id) : Env<RlTraderEnvSpec>(spec, env_id),
                                              foldername(spec.config["foldername"_]),
                                              balance(spec.config["balance"_]),
                                              start_read(spec.config["start"_]),
                                              max_read(spec.config["max"_]),
                                              bidder(10, 0, 9),
                                              asker(10, 0, 9)
  {
    instr_ptr = std::make_unique<Simulator::NormalInstrument>("BTCUSDT", 0.5,
                                                                0.00002, -0.0001, 0.0005);
    int idx = env_id % 45;
    std::string filename = foldername + std::to_string(idx + 1) + ".csv";
    std::cout << filename << std::endl;
    exchange_ptr = std::make_unique<Simulator::Exchange>(filename, 250, start_read, max_read);
    strategy_ptr = std::make_unique<Simulator::Strategy>(*instr_ptr, *exchange_ptr, balance, 0, 0, 20);
    adaptor_ptr = std::make_unique<Simulator::EnvAdaptor>(*strategy_ptr, *exchange_ptr, spec.config["depth"_]);
  }

  void Reset() override {
    steps = 0;
    previous_rpnl = 0;
    previous_upnl = 0;
    previous_fees = 0;
    adaptor_ptr->reset(0, 0);
    timestamp = adaptor_ptr->getTime();
    isDone = false;
    WriteState();
  }

  void Step(const Action& action) override {
      double buy_mean = action["action"_][0];
      double buy_std = action["action"_][1];
      double sell_mean = action["action"_][2];
      double sell_std = action["action"_][3];
      int buy_spread = bidder.mapToDiscrete(buy_mean, buy_std);
      int sell_spread = asker.mapToDiscrete(sell_mean, sell_std);
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
      assert(data.size() == 980*2);
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
