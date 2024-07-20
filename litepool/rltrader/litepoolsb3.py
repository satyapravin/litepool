import pandas as pd
import gymnasium
from typing import Optional
import numpy as np
import torch as th
from packaging import version
from stable_baselines3 import PPO
from stable_baselines3.common.policies import ActorCriticPolicy
from stable_baselines3.common.env_util import make_vec_env
from stable_baselines3.common.evaluation import evaluate_policy
from stable_baselines3.common.vec_env import VecEnvWrapper, VecMonitor
from stable_baselines3.common.vec_env.base_vec_env import (
  VecEnvObs,
  VecEnvStepReturn,
)

import litepool
from litepool.python.protocol import LitePool

class CustomMlpPolicy(ActorCriticPolicy):
    def __init__(self, *args, **kwargs):
        super(CustomMlpPolicy, self).__init__(
            *args,
            **kwargs,
            net_arch=[
                dict(pi=[256, 256], vf=[256, 256])
            ]
        )


class VecAdapter(VecEnvWrapper):
  def __init__(self, venv: LitePool):
    venv.num_envs = venv.spec.config.num_envs
    super().__init__(venv=venv)
    self.mid_prices = []
    self.balances = []
    self.leverages = []
    self.trades = []

  def step_async(self, actions: np.ndarray) -> None:
    self.actions = actions

  def reset(self) -> VecEnvObs:
      return self.venv.reset()[0]

  def seed(self, seed: Optional[int] = None) -> None:
    pass

  def step_wait(self) -> VecEnvStepReturn:
      self.venv.send(self.actions)
    
      obs, rewards, terms, truncs, info_dict = self.venv.recv()
      dones = terms + truncs
      infos = []
      for i in range(len(info_dict["env_id"])):
          infos.append({
              key: info_dict[key][i]
              for key in info_dict.keys()
              if isinstance(info_dict[key], np.ndarray)
          })

          if infos[i]["env_id"] == 0:
              self.mid_prices.append(infos[i]['mid_price'])
              self.balances.append(infos[i]['balance'] + infos[0]['unrealized_pnl'])
              self.leverages.append(infos[i]['leverage'])
              self.trades.append(infos[i]['trade_count'])
 
          if dones[i]:
              if infos[i]["env_id"] == 0:
                  d = {"mid": self.mid_prices, "balance": self.balances, "leverage": self.leverages, "trades": self.trades} 
                  df = pd.DataFrame(d)
                  df.to_csv("temp.csv", header=True, index=False)
                  self.mid_prices = []
                  self.balances = []
                  self.leverages = []
                  self.trades = []
              print('reward = ', rewards[i], '   balance = ',infos[i]['balance'] + infos[i]['unrealized_pnl'], '    drawdown = ', infos[i]['drawdown'])
              infos[i]["terminal_observation"] = obs[i]
              obs[i] = self.venv.reset(np.array([i]))[0]
      return obs, rewards, dones, infos


env = litepool.make("RlTrader-v0", env_type="gymnasium", 
                          num_envs=16, batch_size=16, 
                          num_threads=16, 
                          filename="deribit.csv", 
                          balance=1,
                          depth=20)
env.spec.id = 'RlTrader-v0'
env = VecAdapter(env)
env = VecMonitor(env)

kwargs = dict(use_sde=True, sde_sample_freq=4)

model = PPO(
  CustomMlpPolicy,
  env,
  n_steps=600,
  learning_rate=1e-4,
  gae_lambda=0.95,
  gamma=0.99,
  verbose=0,
  seed=1,
  **kwargs
)

model.learn(200000000)
