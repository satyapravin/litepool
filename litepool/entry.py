"""Entry point for all envs' registration."""

try:
  import litepool.dummy.registration
  #import litepool.simulator.registration
except ImportError:
  pass
