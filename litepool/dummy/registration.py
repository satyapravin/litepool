from litepool.registration import register

register(
  task_id="Dummy-v0",
  import_path="litepool.dummy",
  spec_cls="DummyEnvSpec",
  dm_cls="DummyDMLitePool",
  gym_cls="DummyGymLitePool",
  gymnasium_cls="DummyGymnasiumLitePool",
  max_episode_steps=200,
  reward_threshold=195.0,
)