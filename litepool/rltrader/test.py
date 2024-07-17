import litepool
import numpy as np

num_envs = 64
batch_size = 16
env = litepool.make("RlTrader-v0", env_type="gym", num_envs=num_envs, batch_size=batch_size, filename="data.csv", balance=0.1)
env.async_reset()  # send the initial reset signal to all envs
term = False
while not term:
    obs, rew, term, trunc, info = env.recv()
    env_id = info["env_id"]
    action = np.random.randint(2, size=batch_size)
    print(action)
    env.send(action, env_id)
