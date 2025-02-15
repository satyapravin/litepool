# Copyright 2021 Garena Online Private Limited
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""LitePool Mixin class for meta class definition."""

import pprint
import warnings
from abc import ABC
from typing import Any, Dict, List, Optional, Tuple, Union

import numpy as np
import optree
from dm_env import TimeStep

from .protocol import LitePool, EnvSpec


class LitePoolMixin(ABC):
  """Mixin class for LitePool, exposed to LitePoolMeta."""

  _spec: EnvSpec

  def _check_action(self: LitePool, actions: List[np.ndarray]) -> None:
    if hasattr(self, "_check_action_finished"):  # only check once
      return
    self._check_action_finished = True
    for a, (k, v) in zip(actions, self.spec.action_array_spec.items()):
      if v.dtype != a.dtype:
        raise RuntimeError(
          f"Expected dtype {v.dtype} with action \"{k}\", got {a.dtype}"
        )
      shape = tuple(v.shape)
      if len(shape) > 0 and shape[0] == -1:
        if a.shape[1:] != shape[1:]:
          raise RuntimeError(
            f"Expected shape {shape} with action \"{k}\", got {a.shape}"
          )
      else:
        if len(a.shape) == 0 or a.shape[1:] != shape:
          raise RuntimeError(
            f"Expected shape {('num_env', *shape)} with action \"{k}\", "
            f"got {a.shape}"
          )

  def _from(
    self: LitePool,
    action: Union[Dict[str, Any], np.ndarray],
    env_id: Optional[np.ndarray] = None,
  ) -> List[np.ndarray]:
    """Convert action to C++-acceptable format."""
    if isinstance(action, dict):
      paths, values, _ = optree.tree_flatten_with_path(action)
      adict = {'.'.join(p): v for p, v in zip(paths, values)}
    else:  # only 3 keys in action_keys
      if not hasattr(self, "_last_action_type"):
        self._last_action_type = self._spec._action_spec[-1][0]
      if not hasattr(self, "_last_action_name"):
        self._last_action_name = self._spec._action_keys[-1]
      if isinstance(action, np.ndarray):
        action = action.astype(
          self._last_action_type,  # type: ignore
          order='C',
        )
      adict = {self._last_action_name: action}  # type: ignore
    if env_id is None:
      if "env_id" not in adict:
        adict["env_id"] = self.all_env_ids
    else:
      adict["env_id"] = env_id.astype(np.int32)
    if "players.env_id" not in adict:
      adict["players.env_id"] = adict["env_id"]
    if not hasattr(self, "_action_names"):
      self._action_names = self._spec._action_keys
    return list(map(lambda k: adict[k], self._action_names))  # type: ignore

  def __len__(self: LitePool) -> int:
    """Return the number of environments."""
    return self.config["num_envs"]

  @property
  def all_env_ids(self: LitePool) -> np.ndarray:
    """All env_id in numpy ndarray with dtype=np.int32."""
    if not hasattr(self, "_all_env_ids"):
      self._all_env_ids = np.arange(self.config["num_envs"], dtype=np.int32)
    return self._all_env_ids  # type: ignore

  @property
  def is_async(self: LitePool) -> bool:
    """Return if this env is in sync mode or async mode."""
    return self.config["batch_size"] > 0 and self.config[
      "num_envs"] != self.config["batch_size"]

  def seed(self: LitePool, seed: Optional[Union[int, List[int]]] = None) -> None:
    """Set the seed for all environments (abandoned)."""
    warnings.warn(
      "The `seed` function in litepool is abandoned. "
      "You can set seed by litepool.make(..., seed=seed) instead.",
      stacklevel=2
    )

  def send(
    self: LitePool,
    action: Union[Dict[str, Any], np.ndarray],
    env_id: Optional[np.ndarray] = None,
  ) -> None:
    """Send actions into LitePool."""
    action = self._from(action, env_id)
    self._check_action(action)
    self._send(action)

  def recv(
    self: LitePool,
    reset: bool = False, 
    return_info: bool = True,
  ) -> Union[TimeStep, Tuple]:
    """Recv a batch state from LitePool."""
    try:
        # Add retries for getting state
        max_retries = 3
        retry_count = 0
        state_list = None
        
        while retry_count < max_retries:
            state_list = self._recv()
            # Validate received state
            if state_list and len(state_list) > 0 and all(arr is not None and arr.size > 0 for arr in state_list):
                break
                
            print(f"Retry {retry_count + 1}: Received empty/invalid state")
            retry_count += 1
            time.sleep(0.01)  # Small delay between retries
            
        if not state_list or len(state_list) == 0:
            print("Failed to receive valid state after retries")
            # Return dummy state matching expected format
            return self._to(self._create_dummy_state(), reset, return_info)
            
        return self._to(state_list, reset, return_info)
        
    except Exception as e:
        print(f"Error in recv: {str(e)}")
        return self._to(self._create_dummy_state(), reset, return_info)

  def _create_dummy_state(self) -> List[np.ndarray]:
    """Create a dummy state matching the expected format"""
    # Get expected state structure from spec
    state_spec = self.spec.state_array_spec
    dummy_state = []
    
    for spec_key, spec_value in state_spec.items():
        shape = [s if s != -1 else 1 for s in spec_value.shape]
        if spec_value.dtype == np.float32:
            dummy_arr = np.zeros(shape, dtype=np.float32)
        elif spec_value.dtype == np.int32:
            dummy_arr = np.zeros(shape, dtype=np.int32)
        else:
            dummy_arr = np.zeros(shape, dtype=spec_value.dtype)
        dummy_state.append(dummy_arr)
        
    return dummy_state

  def async_reset(self: LitePool) -> None:
    """Follows the async semantics, reset the envs in env_ids."""
    self._reset(self.all_env_ids)

  def step(
    self: LitePool,
    action: Union[Dict[str, Any], np.ndarray],
    env_id: Optional[np.ndarray] = None,
  ) -> Union[TimeStep, Tuple]:
    """Perform one step with multiple environments in LitePool."""
    self.send(action, env_id)
    return self.recv(reset=False, return_info=True)

  def reset(
    self: LitePool,
    env_id: Optional[np.ndarray] = None,
  ) -> Union[TimeStep, Tuple]:
    """Reset envs in env_id.

    This behavior is not defined in async mode.
    """
    if env_id is None:
      env_id = self.all_env_ids
    self._reset(env_id)
    return self.recv(
      reset=True, return_info=self.config["gym_reset_return_info"]
    )

  @property
  def config(self: LitePool) -> Dict[str, Any]:
    """Config dict of this class."""
    return dict(zip(self._spec._config_keys, self._spec._config_values))

  def __repr__(self: LitePool) -> str:
    """Prettify the debug information."""
    config = self.config
    config_str = ", ".join(
      [f"{k}={pprint.pformat(v)}" for k, v in config.items()]
    )
    return f"{self.__class__.__name__}({config_str})"

  def __str__(self: LitePool) -> str:
    """Prettify the debug information."""
    return self.__repr__()
