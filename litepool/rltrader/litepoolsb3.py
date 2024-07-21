import pandas as pd
import gymnasium
from typing import Optional
import numpy as np
import torch as th
import torch.nn as nn
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
from stable_baselines3.common.torch_layers import create_mlp

import litepool
from litepool.python.protocol import LitePool

import gym
import torch as th
import torch.nn as nn
import numpy as np
from stable_baselines3 import PPO
from stable_baselines3.common.policies import ActorCriticPolicy
from stable_baselines3.common.vec_env import DummyVecEnv

class CustomMLPExtractor(nn.Module):
    def __init__(self, feature_dim, gru_hidden_dim, mlp_hidden_dim):
        super(CustomMLPExtractor, self).__init__()
        self.gru_hidden_dim = gru_hidden_dim
        self.gru = nn.GRU(input_size=feature_dim, hidden_size=gru_hidden_dim, batch_first=True)
        self.policy_net = nn.Sequential(
            nn.Linear(gru_hidden_dim * 2, mlp_hidden_dim),
            nn.ReLU(),
            nn.Linear(mlp_hidden_dim, mlp_hidden_dim),
            nn.ReLU()
        )
        self.value_net = nn.Sequential(
            nn.Linear(gru_hidden_dim * 2, mlp_hidden_dim),
            nn.ReLU(),
            nn.Linear(mlp_hidden_dim, mlp_hidden_dim),
            nn.ReLU()
        )
        self.hidden_state = None
        self.reset_hidden_state()

    def reset_hidden_state(self, batch_size=1):
        self.hidden_state = th.zeros(1, batch_size, self.gru_hidden_dim).to(next(self.gru.parameters()).device)

    def forward_actor(self, x):
        gru_output, self.hidden_state = self.gru(x.unsqueeze(0), self.hidden_state)
        gru_output = gru_output.squeeze(0)
        self.hidden_state = self.hidden_state.detach()
        expanded_hidden_state = self.hidden_state.squeeze(0).expand(x.size(0), -1)
        concatenated_features = th.cat((gru_output, expanded_hidden_state), dim=1)
        return self.policy_net(concatenated_features)

    def forward_critic(self, x):
        gru_output, self.hidden_state = self.gru(x.unsqueeze(0), self.hidden_state)
        gru_output = gru_output.squeeze(0)
        self.hidden_state = self.hidden_state.detach()
        expanded_hidden_state = self.hidden_state.squeeze(0).expand(x.size(0), -1).detach()
        concatenated_features = th.cat((gru_output, expanded_hidden_state), dim=1)
        return self.value_net(concatenated_features)

class CustomGRUPolicy(ActorCriticPolicy):
    def __init__(self, *args, **kwargs):
        super(CustomGRUPolicy, self).__init__(*args, **kwargs, net_arch=[{}])
        
        feature_dim = self.features_extractor.features_dim
        gru_hidden_dim = 16
        mlp_hidden_dim = 16
        self.mlp_extractor = CustomMLPExtractor(feature_dim, gru_hidden_dim, mlp_hidden_dim)

        # Output layers for continuous actions. Assuming action space is Box with shape (2,)
        self.action_net = nn.Linear(mlp_hidden_dim, 2)  # Output two action values for the policy
        self.value_net = nn.Linear(mlp_hidden_dim, 1)  # Output one value for the value network

    def extract_features(self, obs):
        return self.features_extractor(obs)

    def forward(self, obs, deterministic=False):
        features = self.extract_features(obs)
        
        # Pass through the policy and value networks
        policy_latent = self.mlp_extractor.forward_actor(features)
        value_latent = self.mlp_extractor.forward_critic(features)

        action_mean = self.action_net(policy_latent)
        # Scale actions to the range [2, 88]
        actions = 2 + (th.tanh(action_mean) + 1) * 43  # tanh outputs values in [-1, 1], scaling to [2, 88]
        values = self.value_net(value_latent)

        # Calculate log probabilities
        action_dist = th.distributions.Normal(action_mean, 1.0)  # Assuming a fixed stddev for simplicity
        log_probs = action_dist.log_prob(actions).sum(dim=-1)
        
        return actions, values, log_probs

    def _get_action_from_latent(self, latent, deterministic):
        action_mean = self.action_net(latent)
        # Scale actions to the range [2, 88]
        actions = 2 + (th.tanh(action_mean) + 1) * 43  # tanh outputs values in [-1, 1], scaling to [2, 88]
        action_dist = th.distributions.Normal(action_mean, 1.0)  # Assuming a fixed stddev for simplicity
        log_probs = action_dist.log_prob(actions).sum(dim=-1)
        return actions, log_probs

    def _predict(self, observation, deterministic=False):
        actions, _, log_probs = self.forward(observation, deterministic)
        return actions, log_probs

    def evaluate_actions(self, obs, actions):
        features = self.extract_features(obs)
        
        # Pass through the policy and value networks
        policy_latent = self.mlp_extractor.forward_actor(features)
        value_latent = self.mlp_extractor.forward_critic(features)

        action_mean = self.action_net(policy_latent)
        # Scale actions to the range [2, 88]
        action = 2 + (th.tanh(action_mean) + 1) * 43  # tanh outputs values in [-1, 1], scaling to [2, 88]

        action_dist = th.distributions.Normal(action_mean, 1.0)  # Assuming a fixed stddev for simplicity
        log_prob = action_dist.log_prob(actions).sum(dim=-1)
        entropy = action_dist.entropy().sum(dim=-1)
        values = self.value_net(value_latent)
        return values, log_prob, entropy


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

model = PPO(
  CustomGRUPolicy,
  env,
  n_steps=30000,
  learning_rate=1e-3,
  gae_lambda=0.95,
  gamma=0.99,
  verbose=1,
  seed=1,
  **kwargs
)

model.learn(200000000)
