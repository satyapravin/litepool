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
from stable_baselines3.sac.policies import SACPolicy
from typing import Optional, Type
from gymnasium import spaces

device = torch.device("cuda")


import os
env = litepool.make("RlTrader-v0", env_type="gymnasium", 
                          num_envs=1, batch_size=1,
                          num_threads=1,
                          filename="oos.csv", 
                          balance=10000,
                          start=1,
                          max=36000001,
                          depth=20)
env.spec.id = 'RlTrader-v0'

import os

if os.path.exists("sac_rltrader.zip"):
    model = SAC.load("sac_rltrader", env)
    print("saved model loaded")
else:
    print("saved model not found")


obs, info = env.reset()
done = False

counter = 0

while not done:
    action, _states = model.predict(obs, deterministic=True)
    obs, reward, dones, trunc, info = env.step(action)
    
    if dones.any():
        obs = env.reset()
        done = True
        print(info)

    if counter % 1000 == 0:
        print(counter, 'balance = ',info['balance'], "  unreal = ", info['unrealized_pnl'], 
              " real = ", info['realized_pnl'], '    drawdown = ', info['drawdown'], '     fees = ', -info['fees'], 
              '    leverage = ', info['leverage'])
    counter += 1

env.close()
