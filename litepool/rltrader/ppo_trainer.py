import pandas as pd
import gymnasium
from typing import Optional
import numpy as np
import torch as th
import torch.nn as nn
from sb3_contrib import RecurrentPPO
from stable_baselines3.common.policies import ActorCriticPolicy
from stable_baselines3.common.env_util import make_vec_env
from stable_baselines3.common.evaluation import evaluate_policy
from stable_baselines3.common.vec_env import VecEnvWrapper, VecMonitor, VecNormalize
from stable_baselines3.common.vec_env.base_vec_env import (
    VecEnvObs,
    VecEnvStepReturn,
)
from stable_baselines3.common.torch_layers import BaseFeaturesExtractor
from gymnasium import spaces
import litepool
from litepool.python.protocol import LitePool
import os

device = th.device("cuda")

class CustomRecurrentFeatureExtractor(BaseFeaturesExtractor):
    """
    Custom LSTM Feature Extractor for RecurrentPPO.
    This processes observations and applies LSTMs with attention.
    """
    def __init__(self, observation_space: spaces.Box, lstm_hidden_size: int = 16):
        super(CustomRecurrentFeatureExtractor, self).__init__(observation_space, features_dim=lstm_hidden_size * 4)

        self.lstm_hidden_size = lstm_hidden_size
        self.n_input_channels = 38
        self.remaining_input_size = 24

        # Two LSTMs for different input splits
        self.lstm = nn.LSTM(self.n_input_channels, lstm_hidden_size, batch_first=True, bidirectional=True)
        self.lstm2 = nn.LSTM(self.remaining_input_size, lstm_hidden_size, batch_first=True, bidirectional=True)

        # Attention mechanism
        self.attention_weights_layer = nn.Linear(lstm_hidden_size * 2, 1, bias=False)

    def attention_net(self, lstm_output: th.Tensor) -> th.Tensor:
        """
        Apply attention mechanism to LSTM outputs.
        """
        attention_scores = self.attention_weights_layer(lstm_output)  # (batch_size, seq_len, 1)
        attention_weights = th.softmax(attention_scores, dim=1)  # Normalize over the sequence dimension
        context_vector = th.sum(attention_weights * lstm_output, dim=1)  # Weighted sum over the sequence
        return context_vector

    def forward(self, observations: th.Tensor) -> th.Tensor:
        """
        Forward pass for the custom feature extractor.

        Args:
            observations: Tensor of shape (batch_size, seq_len, obs_dim).

        Returns:
            Encoded features for the policy/value networks.
        """
        lstm_input = observations[:, :38 * 10]  # First 10 sequences of 38 features
        remaining_input = observations[:, 38 * 10:]  # Remaining input for the second LSTM

        batch_size = observations.shape[0]
        lstm_input = lstm_input.view(batch_size, 10, 38)  # (batch_size, seq_len, lstm_input_dim)
        remaining_input = remaining_input.view(batch_size, 10, 24)  # (batch_size, seq_len, remaining_input_dim)

        # Process inputs through both LSTMs
        lstm_out, _ = self.lstm(lstm_input)
        context_vector = self.attention_net(lstm_out)

        remaining_out, _ = self.lstm2(remaining_input)
        final_output = th.cat(
            (remaining_out[:, -1, :], context_vector), dim=1
        )  # Concatenate final LSTM output and attention vector

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
                    " real = ", infos[i]['realized_pnl'], '    drawdown = ', infos[i]['drawdown'], '     fees = ', infos[i]['fees'], 
                    ' leverage = ', infos[i]['leverage'])
      self.steps += 1
      return obs, rewards, dones, infos

import os


if os.path.exists("temp.csv"):
    os.remove("temp.csv")

env = litepool.make(
    "RlTrader-v0",
    env_type="gymnasium",
    num_envs=32,
    batch_size=32,
    num_threads=32,
    foldername="./oos/",
    balance=8000,
    start=60000,
    max=360001,
    depth=20,
)
env.spec.id = "RlTrader-v0"

env = VecAdapter(env)
env = VecNormalize(env, norm_obs=True, norm_reward=True)
env = VecMonitor(env)

# RecurrentPPO-specific policy arguments
policy_kwargs = {
    "features_extractor_class": CustomRecurrentFeatureExtractor,
    "features_extractor_kwargs": {"lstm_hidden_size": 16},
    "activation_fn": th.nn.ReLU,
    "net_arch": [32, 32],  # Smaller network for policy/value heads
}

# Initialize RecurrentPPO model
model = RecurrentPPO(
    "MlpLstmPolicy",
    env,
    policy_kwargs=policy_kwargs,
    learning_rate=1e-4,  # Lower learning rate for more stable updates
    n_steps=512,  # Rollout length per environment
    batch_size=64,  # Minibatch size for backpropagation
    n_epochs=4,  # Number of training epochs per update
    gamma=0.99,  # Discount factor
    gae_lambda=0.95,  # GAE parameter
    clip_range=0.2,  # PPO clipping range
    ent_coef=0.01,  # Entropy coefficient
    vf_coef=0.5,  # Value function coefficient
    max_grad_norm=0.5,  # Gradient clipping
    verbose=1,
    device=device,
)

# Train the model
model.learn(total_timesteps=700_000 * 100)
model.save("recurrent_ppo_rltrader")
