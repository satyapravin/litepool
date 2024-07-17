from litepool.registration import register

register(
  task_id="RlTrader-v0",
  import_path="litepool.rltrader",
  spec_cls="RlTraderEnvSpec",
  dm_cls="RlTraderDMLitePool",
  gym_cls="RlTraderGymLitePool",
  gymnasium_cls="RlTraderGymnasiumLitePool",
  max_episode_steps=2000000000000,
  reward_threshold=195000.0,
)
