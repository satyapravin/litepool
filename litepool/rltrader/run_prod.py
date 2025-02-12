import gymnasium
from typing import Optional
import numpy as np
import torch as th
import torch.nn as nn
from packaging import version
from stable_baselines3 import SAC, PPO
from stable_baselines3.common.policies import ActorCriticPolicy
from stable_baselines3.common.env_util import make_vec_env
from stable_baselines3.common.evaluation import evaluate_policy
from stable_baselines3.common.vec_env import VecEnvWrapper, VecMonitor, VecNormalize
from stable_baselines3.common.vec_env.base_vec_env import (
  VecEnvObs,
  VecEnvStepReturn,
)
from stable_baselines3.common.torch_layers import create_mlp
from stable_baselines3.common.torch_layers import BaseFeaturesExtractor
from stable_baselines3.sac.policies import SACPolicy
from stable_baselines3.common.callbacks import BaseCallback

import litepool
from litepool.python.protocol import LitePool

import torch
import torch.nn as nn
import torch.nn.functional as F
from typing import Optional, Type
from gymnasium import spaces

device = torch.device("cpu")

class ResetHiddenStateCallback(BaseCallback):
    def __init__(self, verbose=0):
        super().__init__(verbose)

    def _on_step(self) -> bool:
        dones = self.locals["dones"]  
        for env_index, done in enumerate(dones):
            if done:  
                self.model.actor.features_extractor.reset_hidden_state_for_env(env_index)
        return True

class LSTMFeatureExtractor(BaseFeaturesExtractor):
    def __init__(self, observation_space: spaces.Box, lstm_hidden_size: int = 16):
        super(LSTMFeatureExtractor, self).__init__(observation_space, features_dim=lstm_hidden_size)
        self.lstm_hidden_size = lstm_hidden_size
        self.n_input_channels = 98
        self.lstm = nn.LSTM(self.n_input_channels, lstm_hidden_size, batch_first=True, bidirectional=False).to(device)
        self.hidden = None

    def reset(self):
        self.hidden = None

    def reset_hidden_state_for_env(self, env_idx: int):
        if self.hidden is not None:
            self.hidden = (
                self.hidden[0].detach(),
                self.hidden[1].detach()
            )
            self.hidden[0][:, env_idx, :] = 0  # Reset hidden state (h_0)
            self.hidden[1][:, env_idx, :] = 0  # Reset cell state (c_0)


    def forward(self, observations: torch.Tensor) -> torch.Tensor:
        lstm_input = observations
        batch_size = observations.shape[0]
        lstm_input = lstm_input.view(batch_size, 5, 98)
        if self.hidden is None or lstm_input.shape[0] != self.hidden[0].shape[1]:
            self.hidden = (
                torch.zeros(1, batch_size, self.lstm_hidden_size).to(observations.device),
                torch.zeros(1, batch_size, self.lstm_hidden_size).to(observations.device),
            )
        else:
            self.hidden = (self.hidden[0].detach(), self.hidden[1].detach())

        lstm_out, self.hidden = self.lstm(lstm_input, self.hidden)  # (batch_size, seq_len, hidden_size)
        final_output = lstm_out[:, -1, :]
        return final_output


class VecAdapter(VecEnvWrapper):
  def __init__(self, venv: LitePool):
    venv.num_envs = venv.spec.config.num_envs
    super().__init__(venv=venv)
    self.steps = 0

  def step_async(self, actions: np.ndarray) -> None:
      self.actions = actions 
      self.venv.send(self.actions)

  def reset(self) -> VecEnvObs:
      self.steps = 0
      return self.venv.reset()[0]

  def seed(self, seed: Optional[int] = None) -> None:
     if seed is not None:
          self.venv.seed(seed) 

  def step_wait(self) -> VecEnvStepReturn:
      obs, rewards, terms, truncs, info_dict = self.venv.recv()

     
      if (np.isnan(obs).any() or np.isinf(obs).any()):
          print("NaN in OBS...................")

      if (np.isnan(rewards).any() or np.isinf(rewards).any()):
          print("NaN in REWARDS...................")
          print(rewards)

      dones = terms + truncs
      self.steps += 1
      return obs, rewards, dones, [info_dict]

import os

env = litepool.make("RlTrader-v0", env_type="gymnasium", 
                          num_envs=1, batch_size=1,
                          num_threads=1,
                          is_prod=True,
                          is_inverse_instr=True,
                          api_key="1VOwN5_G",
                          api_secret="GE1U3j-05bHT-zRZ3ZzqtSLRsEyD2_Jobf_JuCbh4l8",
                          symbol="BTC-14FEB25",
                          tick_size=2.5,
                          min_amount=10,
                          maker_fee=-0.0001,
                          taker_fee=0.0005,
                          foldername="./testfiles/", 
                          balance=0.01,
                          start=1,
                          max=72000001,
                          depth=20)
env.spec.id = 'RlTrader-v0'

env = VecAdapter(env)
env = VecNormalize(env, norm_obs=True, norm_reward=True)
env = VecMonitor(env)

import os

custom_objects= {
  'policy_kwargs': {
    'features_extractor_class': LSTMFeatureExtractor,
    'features_extractor_kwargs': {'lstm_hidden_size': 64},
    'share_features_extractor': True,
    'activation_fn': th.nn.ReLU,
    'net_arch': dict(pi=[64, 64], vf=[64, 64], qf=[64, 64]),
  }   
}

from rich.table import Table
from rich.live import Live

def gen_info_table(info: dict):
    table = Table()
    table.add_column("Desc")
    table.add_column("Value")

    if len(info) == 0:
        return table

    table.add_row("Step", str(info['elapsed_step'][0]))
    table.add_row("Realized P/L", str(info['realized_pnl'][0]))
    table.add_row("Position P/L", str(info['unrealized_pnl'][0]))
    table.add_row("Drawdown", str(info['drawdown'][0]))
    table.add_row("Fees", str(info['fees'][0]))
    table.add_row("Leverage", str(info['leverage'][0]))
    table.add_row("Count", str(info['trade_count'][0]))
    return table


if os.path.exists("sac_rltrader.zip"):
    model = SAC.load("sac_rltrader", env, custom_objects=custom_objects)
    print("saved model loaded")

    obs = env.reset()
    done = False

    counter = 0
    with Live(gen_info_table({}), refresh_per_second=1) as live_obj:
        while not done:
            action, _states = model.predict(obs, deterministic=True)
            obs, reward, dones, info = env.step(action)
            live_obj.update(gen_info_table(info[0]))

            if dones.any():
                obs = env.reset()
                done = True

            counter += 1

    env.close()
