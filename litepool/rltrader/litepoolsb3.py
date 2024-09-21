import pandas as pd
import gymnasium
from typing import Optional
import numpy as np
import torch as th
import torch.nn as nn
from packaging import version
from sb3_contrib import RecurrentPPO
from stable_baselines3 import SAC, PPO
from stable_baselines3.common.policies import ActorCriticPolicy
from stable_baselines3.common.env_util import make_vec_env
from stable_baselines3.common.evaluation import evaluate_policy
from stable_baselines3.common.vec_env import VecEnvWrapper, VecMonitor
from stable_baselines3.common.vec_env.base_vec_env import (
  VecEnvObs,
  VecEnvStepReturn,
)
from stable_baselines3.common.torch_layers import create_mlp

import litepool
from litepool.python.protocol import LitePool

import torch
import torch.nn as nn
import torch.nn.functional as F
from stable_baselines3.common.torch_layers import BaseFeaturesExtractor
from gymnasium import spaces

device = torch.device("cuda")

class LSTMFeatureExtractor(BaseFeaturesExtractor):
    def __init__(self, observation_space: spaces.Box, lstm_hidden_size: int = 16, output_size: int = 32):
        # The feature dimension is the final output size of the sequential layer
        super(LSTMFeatureExtractor, self).__init__(observation_space, features_dim=output_size+lstm_hidden_size+24)
        
        self.lstm_hidden_size = lstm_hidden_size
        self.n_input_channels = 38
        self.remaining_input_size = 24 
        self.output_size = output_size
        
        self.lstm = nn.LSTM(self.n_input_channels, lstm_hidden_size, batch_first=True)
        
        # This will hold the hidden state and cell state of the LSTM
        self.hidden = None

        # Define a sequential layer to process the concatenated output
        self.fc = nn.Sequential(
            nn.Linear(self.remaining_input_size, 32),
            nn.ReLU(),
            nn.Linear(32, output_size)
        )

    def forward(self, observations: torch.Tensor) -> torch.Tensor:
        # Split the observations into two parts
        lstm_input = observations[:, :38]
        remaining_input = observations[:, 38:]

        # Initialize hidden state and cell state with zeros if they are not already set
        if self.hidden is None or lstm_input.shape[0] != self.hidden[0].shape[1]:
            self.hidden = (torch.zeros(1, lstm_input.shape[0], self.lstm_hidden_size).to(observations.device),
                           torch.zeros(1, lstm_input.shape[0], self.lstm_hidden_size).to(observations.device))
        else:
            # Detach the hidden state to avoid backpropagating through the entire history
            self.hidden = (self.hidden[0].detach(), self.hidden[1].detach())
        
        # LSTM expects input of shape (batch_size, seq_len, input_size)
        lstm_input = lstm_input.unsqueeze(1)  # Add sequence dimension
        lstm_out, self.hidden = self.lstm(lstm_input, self.hidden)
        
        lstm_hidden_state = lstm_out[:, -1, :]
        
        final_output = self.fc(remaining_input)
        combined_output = torch.cat((lstm_hidden_state, final_output, remaining_input), dim=1)
        return combined_output 

class VecAdapter(VecEnvWrapper):
  def __init__(self, venv: LitePool):
    venv.num_envs = venv.spec.config.num_envs
    super().__init__(venv=venv)
    self.mid_prices = []
    self.balances = []
    self.leverages = []
    self.trades = []
    self.fees = []
    self.buys = []
    self.sells = []
    self.upnl = []
    self.steps = 0
    self.header = True

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
              self.balances.append(infos[i]['balance'])
              self.upnl.append(infos[i]['unrealized_pnl'])
              self.leverages.append(infos[i]['leverage'])
              self.trades.append(infos[i]['trade_count'])
              self.fees.append(infos[i]['fees'])
              self.buys.append(infos[i]['buy_amount'])
              self.sells.append(infos[i]['sell_amount']) 
           
          if dones[i]:
              infos[i]["terminal_observation"] = obs[i]
              obs[i] = self.venv.reset(np.array([i]))[0]

          if self.steps % 1000 == 0 or dones[i]:
              if infos[i]["env_id"] == 0:
                  d = {"mid": self.mid_prices, "balance": self.balances, "upnl" : self.upnl, 
                       "leverage": self.leverages, "trades": self.trades, "fees": self.fees,
                       "buy_amount": self.buys, "sell_amount": self.sells } 
                  df = pd.DataFrame(d)

                  if self.header:
                       df.to_csv("temp.csv", header=self.header, index=False) 
                       self.header=False
                  else:
                       df.to_csv("temp.csv", mode='a', header=False, index=False)

                  self.mid_prices = []
                  self.balances = []
                  self.upnl = []
                  self.leverages = []
                  self.trades = []
                  self.fees = []
                  self.buys = []
                  self.sells = [] 
                  self.header = False
                  self.header = dones[i]
                  print("env_id ", i,  " steps ", self.steps, 'balance = ',infos[i]['balance'], "  unreal = ", infos[i]['unrealized_pnl'], 
                    " real = ", infos[i]['realized_pnl'], '    drawdown = ', infos[i]['drawdown'])
      self.steps += 1
      if (np.isnan(obs).any()):
          print("NaN in OBS...................")
      return obs, rewards, dones, infos

import os
if os.path.exists('temp.csv'):
    os.remove('temp.csv')
env = litepool.make("RlTrader-v0", env_type="gymnasium", 
                          num_envs=64, batch_size=64, 
                          num_threads=64,
                          filename="bitmex.csv", 
                          balance=2000,
                          depth=20)
env.spec.id = 'RlTrader-v0'
env = VecAdapter(env)
env = VecMonitor(env)

kwargs = dict(use_sde=True, sde_sample_freq=4)

policy_kwargs = {
    'features_extractor_class': LSTMFeatureExtractor,
    'features_extractor_kwargs': {
        'lstm_hidden_size': 64,
        'output_size': 32
    },
    'activation_fn': th.nn.ReLU,
    'net_arch': dict(pi=[64, 128, 32], vf=[64, 128, 32], qf=[64, 128, 32])
}

import os

'''
model = PPO(
  "MlpPolicy", 
  env,
  policy_kwargs=policy_kwargs,
  learning_rate=1e-4,
  n_steps=1024, 
  batch_size=64,
  gamma=0.99,
  clip_range=0.1,
  ent_coef=0.02,
  vf_coef=0.5,
  max_grad_norm=0.5,
  #target_kl=0.1,
  gae_lambda=0.95,
  verbose=2,
  seed=10,
  **kwargs
)
'''

model = SAC(
    "MlpPolicy",
    env,
    policy_kwargs=policy_kwargs,
    learning_rate=3e-4,
    buffer_size=1000000,
    learning_starts=200,
    batch_size=4096,
    tau=0.005,
    gamma=0.99,
    train_freq=1,
    gradient_steps=1,
    ent_coef='auto',
    target_update_interval=1,
    target_entropy='auto',
    verbose=2,
    device=device,
)

model.learn(200000000)
