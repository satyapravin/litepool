from litepool.registration import register

register(
  task_id="RlTrader-v0",
  import_path="litepool.dummy",
  spec_cls="RlTraderEnvSpec",
  dm_cls="RlTraderDMLitePool",
  gym_cls="RlTraderGymLitePool",
  gymnasium_cls="RlTraderGymnasiumLitePool",
  max_episode_steps=200,
  reward_threshold=195.0,
)
