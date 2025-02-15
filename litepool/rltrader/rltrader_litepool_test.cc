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

#include "litepool/rltrader/rltrader_litepool.h"

#include <gtest/gtest.h>

#include <random>
#include <vector>

using RlTraderAction = rltrader::RlTraderEnv::Action;
using RlTraderState = rltrader::RlTraderEnv::State;

TEST(RlTraderLitePoolTest, SplitZeroAction) {
  auto config = rltrader::RlTraderEnvSpec::kDefaultConfig;
  int num_envs = 4;
  config["num_envs"_] = num_envs;
  config["batch_size"_] = 4;
  config["num_threads"_] = 1;
  config["seed"_] = 42;

  rltrader::RlTraderEnvSpec spec(config);
  rltrader::RlTraderLitePool litepool(spec);

  TArray all_env_ids(Spec<int>({num_envs}));
  for (int i = 0; i < num_envs; ++i) {
    all_env_ids[i] = i;
  }

  litepool.Reset(all_env_ids);
  auto state_vec = litepool.Recv();
  // construct action

  std::vector<Array> raw_action({Array(Spec<int>({4})),
                                   Array(Spec<int>({4, 1})),
                                   Array(Spec<int>({4, 6}))});

  RlTraderAction action(raw_action);

  for (int i = 0; i < 4; ++i) {
    action["env_id"_][i] = i;
    action["action"_][i][0] = 5;
    action["action"_][i][1] = 5;
    action["action"_][i][2] = 80;
    action["action"_][i][3] = 80;
    action["action"_][i][4] = 2;
    action["action"_][i][5] = 3;
  }

  litepool.Send(action);


  RlTraderState state(litepool.Recv());

  //EXPECT_EQ(state["info:players.env_id"_].Shape(0), 8);
  for (int i = 0; i < 4; ++i) {
    EXPECT_EQ(static_cast<int>(state["info:env_id"_][i]), i);
    auto obs = state["obs"_](i);
    EXPECT_EQ(obs.size, 196);
  }
}

void Runner(int num_envs, int batch, int seed, int total_iter, int num_threads) {
  auto config = rltrader::RlTraderEnvSpec::kDefaultConfig;
  config["num_envs"_] = num_envs;
  config["batch_size"_] = batch;
  config["num_threads"_] = num_threads;
  config["seed"_] = seed;
  config["max_num_players"_] = 1;

  std::vector<int> length;

  length.reserve(num_envs);
  for (int i = 0; i < num_envs; ++i) {
    length.push_back(seed + i);
  }
  rltrader::RlTraderEnvSpec spec(config);
  rltrader::RlTraderLitePool litepool(spec);
  TArray all_env_ids(Spec<int>({num_envs}));
  for (int i = 0; i < num_envs; ++i) {
      all_env_ids[i] = i;
  }
  litepool.Reset(all_env_ids);

  auto start = std::chrono::system_clock::now();
  for (int ii = 0; ii < total_iter; ++ii) {
    // recv
    RlTraderState state(litepool.Recv());
    // check state
    auto env_id = state["info:env_id"_];
    auto obs = state["obs"_];
    auto reward = state["reward"_];
    auto done = state["done"_];

    std::vector<Array> raw_action({Array(Spec<int>({num_envs})),
                                    Array(Spec<int>({num_envs, 1})),
                                    Array(Spec<int>({num_envs, 6}))});
    RlTraderAction action(raw_action);

    for (int i = 0; i < num_envs; ++i) {
      action["env_id"_][i] = i;
      action["action"_][i][0] = 5;
      action["action"_][i][1] = 5;
      action["action"_][i][2] = 80;
      action["action"_][i][3] = 80;
      action["action"_][i][4] = 3;
      action["action"_][i][5] = 2;
    }
    litepool.Send(action);
  }
  std::chrono::duration<double> dur = std::chrono::system_clock::now() - start;
  double t = dur.count();
  double fps = (total_iter * batch) / t;
  std::cout << "time(s): " << t << ", FPS: " << fps << std::endl;
}

TEST(RlTraderLitePoolTest, SinglePlayer) {
  Runner(1, 1, 20, 100, 1);
  Runner(3, 1, 22, 1000, 3);
  Runner(12, 3, 21, 1000, 9);
}
