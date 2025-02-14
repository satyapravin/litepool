# Copyright 2021 Garena Online Private Limited
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Unit test for dummy litepool and speed benchmark."""

import os
import time

import numpy as np
from absl import logging
from absl.testing import absltest
from litepool.rltrader.rltrader_litepool import _RlTraderLitePool, _RlTraderEnvSpec


class _RlTraderLitePoolTest(absltest.TestCase):

  def test_config(self) -> None:
    ref_config_keys = [
      "num_envs",
      "batch_size",
      "num_threads",
      "max_num_players",
      "thread_affinity_offset",
      "seed",
      "gym_reset_return_info",
      "max_episode_steps",
    ]
    default_conf = _RlTraderEnvSpec._default_config_values
    self.assertTrue(isinstance(default_conf, tuple))
    config_keys = _RlTraderEnvSpec._config_keys
    self.assertTrue(isinstance(config_keys, list))
    self.assertEqual(len(default_conf), len(config_keys))
    self.assertEqual(sorted(config_keys), sorted(ref_config_keys))
    print("test_config completed")

  def test_spec(self) -> None:
    conf = _RlTraderEnvSpec._default_config_values
    env_spec = _RlTraderEnvSpec(conf)
    state_spec = env_spec._state_spec
    action_spec = env_spec._action_spec
    state_keys = env_spec._state_keys
    action_keys = env_spec._action_keys
    self.assertTrue(isinstance(state_spec, tuple))
    self.assertTrue(isinstance(action_spec, tuple))
    state_spec = dict(zip(state_keys, state_spec))
    action_spec = dict(zip(action_keys, action_spec))
    # default value of state_num is 10
    print(state_spec["obs"])
    # change conf and see if it can successfully change state_spec
    # directly send dict or expose config as dict?
    conf = dict(zip(_RlTraderEnvSpec._config_keys, conf))
    env_spec = _RlTraderEnvSpec(tuple(conf.values()))
    state_spec = dict(zip(state_keys, env_spec._state_spec))
    print("test_spec completed")

  def test_litepool(self) -> None:
    conf = dict(
      zip(_RlTraderEnvSpec._config_keys, _RlTraderEnvSpec._default_config_values)
    )
    conf["num_envs"] = num_envs = 100
    conf["batch_size"] = batch = 20
    conf["num_threads"] = os.cpu_count()
    env_spec = _RlTraderEnvSpec(tuple(conf.values()))
    env = _RlTraderLitePool(env_spec)
    state_keys = env._state_keys
    total = 600
    env._reset(np.arange(num_envs, dtype=np.int32))
    t = time.time()
    for iter in range(total):
      print("total", total, "iter=", iter)
      state = env._recv()
      print(state)

      action = {
        "env_id": info["env_id"],
        "buy_angle" : 40,
        "sell_angle": 48
      }
      env._send(tuple(action.values()))
      state = env._recv()
      print(state)
    duration = time.time() - t
    fps = total * batch / duration
    logging.info(f"FPS = {fps:.6f}")


if __name__ == "__main__":
  absltest.main()
