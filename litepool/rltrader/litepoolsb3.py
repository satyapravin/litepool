import pandas as pd
import gymnasium
from typing import Optional
import numpy as np
import torch as th
import torch.nn as nn
from packaging import version
#from sb3_contrib import RecurrentPPO
from stable_baselines3 import PPO
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


class LSTMFeatureExtractor(BaseFeaturesExtractor):
    def __init__(self, observation_space: spaces.Box, lstm_hidden_size: int = 128, output_size: int = 32):
        # The feature dimension is the final output size of the sequential layer
        super(LSTMFeatureExtractor, self).__init__(observation_space, features_dim=output_size)
        
        self.lstm_hidden_size = lstm_hidden_size
        self.n_input_channels = 210
        self.remaining_input_size = 48
        self.output_size = output_size
        
        # Define the LSTM layer for the first 210 floats
        self.lstm = nn.LSTM(self.n_input_channels, lstm_hidden_size, batch_first=True)
        
        # This will hold the hidden state and cell state of the LSTM
        self.hidden = None

        # Define a sequential layer to process the concatenated output
        self.fc = nn.Sequential(
            nn.Linear(lstm_hidden_size + self.remaining_input_size, 128),
            nn.ReLU(),
            nn.Linear(128, output_size)
        )

    def forward(self, observations: torch.Tensor) -> torch.Tensor:
        # Split the observations into two parts
        lstm_input = observations[:, :210]
        remaining_input = observations[:, 210:]

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
        
        # Get the hidden state from the last LSTM cell
        lstm_hidden_state = lstm_out[:, -1, :]
        
        # Concatenate the LSTM hidden state with the remaining input
        combined_output = torch.cat((lstm_hidden_state, remaining_input), dim=1)
        
        # Pass the combined output through the fully connected layer
        final_output = self.fc(combined_output)
        
        return final_output


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
              self.fees.append(infos[i]['fees'])
              self.buys.append(infos[i]['buy_amount'])
              self.sells.append(infos[i]['sell_amount']) 
 
          if dones[i]:
              if infos[i]["env_id"] == 0:
                  d = {"mid": self.mid_prices, "balance": self.balances, "leverage": self.leverages, "trades": self.trades, "fees": self.fees,
                       "buy_amount": self.buys, "sell_amount": self.sells } 
                  df = pd.DataFrame(d)
                  df.to_csv("temp.csv", header=True, index=False)
                  self.mid_prices = []
                  self.balances = []
                  self.leverages = []
                  self.trades = []
                  self.fees = []
                  self.buys = []
                  self.sells = [] 
              print('reward = ', rewards[i], '   balance = ',infos[i]['balance'] + infos[i]['unrealized_pnl'], '    drawdown = ', infos[i]['drawdown'])
              infos[i]["terminal_observation"] = obs[i]
              obs[i] = self.venv.reset(np.array([i]))[0]
      return obs, rewards, dones, infos


env = litepool.make("RlTrader-v0", env_type="gymnasium", 
                          num_envs=4, batch_size=4, 
                          num_threads=4, 
                          filename="deribit.csv", 
                          balance=1,
                          depth=20)
env.spec.id = 'RlTrader-v0'
env = VecAdapter(env)
env = VecMonitor(env)

kwargs = dict(use_sde=True, sde_sample_freq=4)

policy_kwargs = {
    'features_extractor_class': LSTMFeatureExtractor,
    'features_extractor_kwargs': {
        'lstm_hidden_size': 128,
        'output_size': 32
    }
}

model = PPO(
  "MlpPolicy", #CustomGRUPolicy,
  env,
  policy_kwargs=policy_kwargs,
  n_steps=4800,
  learning_rate=1e-4,
  gae_lambda=0.95,
  gamma=0.99,
  verbose=1,
  seed=1,
  **kwargs
)

model.learn(200000000)
