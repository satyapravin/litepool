"""Entry point for all envs' registration."""

try:
  import litepool.dummy.registration
  import litepool.rltrader.registration
except ImportError:
  pass
