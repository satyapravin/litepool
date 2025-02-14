import pandas as pd
import gymnasium
from typing import Optional
import numpy as np
import torch
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
from stable_baselines3.common.callbacks import BaseCallback

device = torch.device("cuda")

class ResetHiddenStateCallback(BaseCallback):
    def __init__(self, verbose=0):
        super().__init__(verbose)

    def _on_step(self) -> bool:
        dones = self.locals["dones"]
        for env_index, done in enumerate(dones):
            if done:
                self.model.policy.features_extractor.reset_hidden_state_for_env(env_index)
        return True


class LSTMFeatureExtractor(BaseFeaturesExtractor):
    def __init__(self, observation_space: spaces.Box, lstm_hidden_size: int = 16):
        super(LSTMFeatureExtractor, self).__init__(observation_space, features_dim=lstm_hidden_size*2)

        self.lstm_hidden_size = lstm_hidden_size
        self.n_input_channels = 38
        self.remaining_input_size = 24

        self.lstm = nn.LSTM(self.n_input_channels, lstm_hidden_size, batch_first=True, bidirectional=False).to(device)
        self.lstm2 = nn.LSTM(self.remaining_input_size, lstm_hidden_size, batch_first=True, bidirectional=False).to(device)

        # This will hold the hidden state and cell state of the LSTM
        self.hidden = None
        self.hidden2 = None


    def reset(self):
        self.hidden = None
        self.hidden2 = None


    def reset_hidden_state_for_env(self, env_idx: int):
        if self.hidden is not None:
            self.hidden = (
                self.hidden[0].detach(),
                self.hidden[1].detach()
            )
            self.hidden[0][:, env_idx, :] = 0  # Reset hidden state (h_0)
            self.hidden[1][:, env_idx, :] = 0  # Reset cell state (c_0)

        if self.hidden2 is not None:
            self.hidden2 = (
                self.hidden2[0].detach(),
                self.hidden2[1].detach()
            )
            self.hidden2[0][:, env_idx, :] = 0  # Reset hidden state (h_0)
            self.hidden2[1][:, env_idx, :] = 0  # Reset cell state (c_0)

    def forward(self, observations: torch.Tensor) -> torch.Tensor:
        lstm_input = observations[:, :38]  # First 3 sequences of 38 features
        remaining_input = observations[:, 38:]  # Remaining input for the second LSTM

        batch_size = observations.shape[0]
        lstm_input = lstm_input.view(batch_size, 1, 38)  # (batch_size, seq_len, input_size)
        remaining_input = remaining_input.view(batch_size, 1, 24)

        if self.hidden is None or lstm_input.shape[0] != self.hidden[0].shape[1]:
            self.hidden = (
                torch.zeros(1, batch_size, self.lstm_hidden_size).to(observations.device),
                torch.zeros(1, batch_size, self.lstm_hidden_size).to(observations.device),
            )
        else:
            self.hidden = (self.hidden[0].detach(), self.hidden[1].detach())

        if self.hidden2 is None or remaining_input.shape[0] != self.hidden2[0].shape[1]:
            self.hidden2 = (
                torch.zeros(1, batch_size, self.lstm_hidden_size).to(observations.device),
                torch.zeros(1, batch_size, self.lstm_hidden_size).to(observations.device),
            )
        else:
            self.hidden2 = (self.hidden2[0].detach(), self.hidden2[1].detach())

        lstm_out, self.hidden = self.lstm(lstm_input, self.hidden)  # (batch_size, seq_len, hidden_size)
        remaining_out, self.hidden2 = self.lstm2(remaining_input, self.hidden2)  # (batch_size, seq_len, hidden_size)

        final_output = torch.cat((
            remaining_out[:, -1, :],    # Final output of the second LSTM (last time step)
            lstm_out[:, -1, :]
        ), dim=1)  # Concatenate along the feature dimension
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
    num_envs=64,
    batch_size=64,
    num_threads=64,
    foldername="./oos/",
    balance=4000,
    start=1,
    max=360001,
    depth=20,
)
env.spec.id = "RlTrader-v0"

env = VecAdapter(env)
env = VecNormalize(env, norm_obs=True, norm_reward=True)
env = VecMonitor(env)
print(env.action_space)

policy_kwargs = {
    'features_extractor_class': LSTMFeatureExtractor,
    'features_extractor_kwargs': {'lstm_hidden_size': 64},
    'share_features_extractor': True,
    'net_arch': dict(pi=[64, 64], vf=[64, 64], qf=[64, 64]),
}

if os.path.exists("recurrent_ppo_rltrader.zip"):
    model = RecurrentPPO.load("recurrent_ppo_rltrader")
    model.set_env(env)
    print("saved model loaded")
else:
    model = RecurrentPPO(
        "MlpLstmPolicy",
        env,
        policy_kwargs=policy_kwargs,
        learning_rate=5e-4,  # Lower learning rate for more stable updates
        n_steps=2048,        # Rollout length per environment
        batch_size=4096,     # Minibatch size for backpropagation
        n_epochs=10,     # Number of training epochs per update
        gamma=0.95,      # Discount factor
        gae_lambda=0.95, # GAE parameter
        clip_range=0.2,  # PPO clipping range
        ent_coef=0.02,   # Entropy coefficient
        vf_coef=0.5,     # Value function coefficient
        max_grad_norm=0.5, # Gradient clipping
        normalize_advantage=True,
        verbose=1,
        device=device,
    )

print(model.policy.action_dist)
model.learn(total_timesteps=700_000 * 100, callback=ResetHiddenStateCallback())
model.save("recurrent_ppo_rltrader")
