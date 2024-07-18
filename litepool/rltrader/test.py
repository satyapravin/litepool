import litepool
import numpy as np

num_envs = 64
batch_size = 4 
env = litepool.make("RlTrader-v0", env_type="gym", num_envs=num_envs, batch_size=batch_size, num_threads=16, filename="data.csv", balance=0.1)
env.async_reset()  # send the initial reset signal to all envs
term = False

counter = 0
while True:
    obs, rew, term, info = env.recv()
    env_id = info["env_id"]
    buy_angle = np.zeros(len(env_id), dtype=np.float32)
    sell_angle = np.zeros(len(env_id), dtype=np.float32)

    for i in range(len(env_id)):
        buy_angle[i] = 45.0
        sell_angle[i] = 45.0
    action = {"buy_angle": buy_angle, "sell_angle": sell_angle}
    env.send(action, env_id)
    counter += 1
    print(rew)
