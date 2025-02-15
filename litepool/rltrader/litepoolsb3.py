import pandas as pd
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

device = torch.device("cuda")



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
        lstm_input = lstm_input.view(batch_size, 2, 98)  
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
    self.action_env_ids = np.arange(self.venv.num_envs, dtype=np.int32)

  def step_async(self, actions: np.ndarray) -> None:
      self.actions = actions 
      self.venv.send(self.actions, self.action_env_ids)

  def reset(self) -> VecEnvObs:
      self.steps = 0
      return self.venv.reset(self.action_env_ids)[0]

  def seed(self, seed: Optional[int] = None) -> None:
     if seed is not None:
          self.venv.seed(seed) 

  def step_wait(self) -> VecEnvStepReturn:
      obs, rewards, terms, truncs, info_dict = self.venv.recv()
      print("OBS", obs)
      print("INfo", info_dict)
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

          if True:
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

          if self.steps % 1 == 0 or dones[i]:
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
          print("id ", infos[i]["env_id"],  " steps ", self.steps, 'balance=',infos[i]['balance'] - infos[i]['fees'], "  unreal=", infos[i]['unrealized_pnl'], 
                    " real=", infos[i]['realized_pnl'], '    drawdown=', infos[i]['drawdown'], '     fees=', infos[i]['fees'], 
                    ' leverage=', infos[i]['leverage'])
      self.steps += 1
      return obs, rewards, dones, infos

import os
if os.path.exists('temp.csv'):
    os.remove('temp.csv')
env = litepool.make("RlTrader-v0", env_type="gymnasium", 
                          num_envs=2, batch_size=2,
                          num_threads=2,
                          is_prod=False,
                          is_inverse_instr=True,
                          api_key="",
                          api_secret="",
                          symbol="BTC-PERPETUAL",
                          tick_size=0.5,
                          min_amount=10,
                          maker_fee=-0.0000001,
                          taker_fee=0.0005,
                          foldername="./train_files/", 
                          balance=0.01,
                          start=100000,
                          max=720001)

env.spec.id = 'RlTrader-v0'

env = VecAdapter(env)
env = VecNormalize(env, norm_obs=True, norm_reward=True)
env = VecMonitor(env)

kwargs = dict(use_sde=True, sde_sample_freq=4)

policy_kwargs = {
    'features_extractor_class': LSTMFeatureExtractor,
    'features_extractor_kwargs': {'lstm_hidden_size': 64},
    'share_features_extractor': True,
    'activation_fn': th.nn.ReLU,
    'net_arch': dict(pi=[64, 64], vf=[64, 64], qf=[64, 64]),
}

import os
from stable_baselines3.common.noise import NormalActionNoise


if os.path.exists("sac_rltrader.zip"):
    model = SAC.load("sac_rltrader")
    model.load_replay_buffer("replay_buffer.pkl")
    model.set_env(env)
    print("saved model loaded")
else:
    model = SAC("MlpPolicy",
                env,
                policy_kwargs=policy_kwargs,
                learning_rate=5e-4,                 # Lower learning rate for more stable updates
                buffer_size=1000000,                # Smaller buffer to focus on more relevant recent experiences
                learning_starts=100,                # Delay learning to allow for better exploration initially
                batch_size=512,                     # Larger batch size for more stable gradient updates
                gamma=0.99,                         # Higher gamma to focus more on long-term reward
                train_freq=256,                     # Train less frequently to prevent overfitting to noise
                gradient_steps=64,                  # Increase gradient steps per update for more efficient learning
                tau=0.01,                           # speed of main to target network
                ent_coef='auto',                    # Keep auto-tuning for entropy, but monitor its value
                target_entropy='auto',              # Consider manually setting a higher target entropy if needed
                verbose=1,
                device=device)

model.learn(700000 * 10, callback=ResetHiddenStateCallback())
model.save("sac_rltrader")
model.save_replay_buffer("replay_buffer.pkl")
