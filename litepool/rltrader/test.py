import litepool
import numpy as np

num_envs = 64
batch_size = 4 
env = litepool.make("RlTrader-v0", env_type="gym", num_envs=num_envs, batch_size=batch_size, num_threads=16, filename="deribit.csv", balance=0.1, depth=20)
env.async_reset()  # send the initial reset signal to all envs
term = False

counter = 0
while True:
    obs, rew, term, info = env.recv()
    env_id = info["env_id"]
    action = np.zeros((len(env_id), 2))
 
    for i in range(len(env_id)):
        action[i][0] = 45.0
        action[i][1] = 45.0
    env.send(action, env_id)
    counter += 1
    print(counter, rew)
