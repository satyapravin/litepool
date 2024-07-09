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

namespace rltrader {

class RlTraderEnvFns {
 public:
  static decltype(auto) DefaultConfig() {
    return MakeDict("state_num"_.Bind(10), "action_num"_.Bind(6));
  }

  template <typename Config>
  static decltype(auto) StateSpec(const Config& conf) {
    return MakeDict("obs:raw"_.Bind(Spec<int>({-1, conf["state_num"_]})),
                    "obs:dyn"_.Bind(Spec<Container<int>>(
                        {-1}, Spec<int>({-1, conf["state_num"_]}))),
                    "info:players.done"_.Bind(Spec<bool>({-1})),
                    "info:players.id"_.Bind(
                        Spec<int>({-1}, {0, conf["max_num_players"_]})));
  }

  template <typename Config>
  static decltype(auto) ActionSpec(const Config& conf) {
    return MakeDict("list_action"_.Bind(Spec<double>({6})),
                    "players.action"_.Bind(Spec<int>({-1})),
                    "players.id"_.Bind(Spec<int>({-1})));
  }
};

using RlTraderEnvSpec = EnvSpec<RlTraderEnvFns>;

class RlTraderEnv : public Env<RlTraderEnvSpec> {
 protected:
  int state_{0};

 public:
  RlTraderEnv(const Spec& spec, int env_id) : Env<RlTraderEnvSpec>(spec, env_id) {
    if (seed_ < 1) {
      seed_ = 1;
    }
  }

  void Reset() override {
  }

  void Step(const Action& action) override {
  }

  bool IsDone() override { return state_ >= seed_; }
};

using RlTraderLitePool = AsyncLitePool<RlTraderEnv>;

}  // namespace rltrader

#endif  // LITEPOOL_RLTRADER_RLTRADER_LITEPOOL_H_
