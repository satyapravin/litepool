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
from stable_baselines3.common.vec_env import VecEnvWrapper, VecMonitor, VecNormalize
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
from stable_baselines3.sac.policies import SACPolicy
from typing import Optional, Type
from gymnasium import spaces

device = torch.device("cuda")


class LSTMFeatureExtractor(BaseFeaturesExtractor):
    def __init__(self, observation_space: spaces.Box, lstm_hidden_size: int = 16, output_size: int = 32):
        # The feature dimension is the final output size of the sequential layer
        super(LSTMFeatureExtractor, self).__init__(observation_space, features_dim=output_size+lstm_hidden_size+24)
        
        self.lstm_hidden_size = lstm_hidden_size
        self.n_input_channels = 62*4 
        self.remaining_input_size = 24*4 
        self.output_size = output_size
        
        self.lstm = nn.LSTM(self.n_input_channels, lstm_hidden_size, batch_first=True).to(device)
        
        # This will hold the hidden state and cell state of the LSTM
        self.hidden = None

        # Define a sequential layer to process the concatenated output
        self.fc = nn.Sequential(
            nn.Linear(self.remaining_input_size, 128),
            nn.ReLU(),
            nn.Linear(128, 64),
            nn.ReLU(),
            nn.Linear(64, output_size),
            nn.ReLU()
        ).to(device)
   
    def reset(self):
        self.hidden = None

    def reset_hidden_state_for_env(self, env_idx: int):
        if self.hidden is not None:
            self.hidden[0][:, env_idx, :] = 0  # Reset hidden state (h_0) for environment `env_idx`
            self.hidden[1][:, env_idx, :] = 0  # Reset cell state (c_0) for environment `env_idx` 
    
    def forward(self, observations: torch.Tensor) -> torch.Tensor:
        # Split the observations into two parts
        lstm_input = observations
        remaining_input = observations[:, 38*4:]

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
  def __init__(self, venv: LitePool, featureExtractor):
    venv.num_envs = venv.spec.config.num_envs
    super().__init__(venv=venv)
    self.featureExtractor = featureExtractor
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
      self.featureExtractor.reset()
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
              self.featureExtractor.reset_hidden_state_for_env(i)

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
                    " real = ", infos[i]['realized_pnl'], '    drawdown = ', infos[i]['drawdown'], '     fees = ', -infos[i]['fees'], 
                    ' leverage = ', infos[i]['leverage'])
      self.steps += 1
      return obs, rewards, dones, infos

import os
env = litepool.make("RlTrader-v0", env_type="gymnasium", 
                          num_envs=1, batch_size=1,
                          num_threads=1,
                          filename="oos.csv", 
                          balance=20000,
                          start=1,
                          max=72000001,
                          depth=20)
env.spec.id = 'RlTrader-v0'

feature_extractor = LSTMFeatureExtractor(env.observation_space, lstm_hidden_size=32, output_size=32)

env = VecAdapter(env, feature_extractor)
env = VecNormalize(env, norm_obs=True, norm_reward=True)
env = VecMonitor(env)
env = VecNormalize(env)


if os.path.exists("sac_rltrader.zip"):
    model = SAC.load("sac_rltrader", env)
    print("saved model loaded")
else:
    print("saved model not found")


obs = env.reset()
done = False

counter = 0

while not done:
    action, _states = model.predict(obs, deterministic=True)
    obs, reward, dones, info = env.step(action)

    if dones.any():
        obs = env.reset()
        done = True
        print(info)

    counter += 1

env.close()

