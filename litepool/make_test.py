"""Test for litepool.make."""

import pprint
from typing import List

import dm_env
import gym
import gymnasium
from absl.testing import absltest

import litepool


class _MakeTest(absltest.TestCase):

  def test_version(self) -> None:
    print(litepool.__version__)

  def test_list_all_envs(self) -> None:
    pprint.pprint(litepool.list_all_envs())


if __name__ == "__main__":
  absltest.main()
