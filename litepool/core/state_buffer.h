// state_buffer.h
#ifndef LITEPOOL_CORE_STATE_BUFFER_H_
#define LITEPOOL_CORE_STATE_BUFFER_H_

#ifndef MOODYCAMEL_DELETE_FUNCTION
#define MOODYCAMEL_DELETE_FUNCTION = delete
#endif

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <functional>
#include <utility>
#include <vector>

#include "litepool/core/array.h"
#include "litepool/core/dict.h"
#include "litepool/core/spec.h"
#include <concurrentqueue/moodycamel/lightweightsemaphore.h>

class StateBuffer {
 protected:
  std::size_t batch_;
  std::size_t max_num_players_;
  std::vector<Array> arrays_;
  std::vector<bool> is_player_state_;
  std::atomic<uint64_t> offsets_{0};
  std::atomic<std::size_t> alloc_count_{0};
  std::atomic<std::size_t> done_count_{0};
  moodycamel::LightweightSemaphore sem_;

 public:
  struct WritableSlice {
    std::vector<Array> arr;
    std::function<void()> done_write;
  };

  StateBuffer(std::size_t batch, std::size_t max_num_players,
              const std::vector<ShapeSpec>& specs,
              std::vector<bool> is_player_state)
      : batch_(batch),
        max_num_players_(max_num_players),
        arrays_(MakeArray(specs)),
        is_player_state_(std::move(is_player_state)) {}

  WritableSlice Allocate(std::size_t num_players, int order = -1) {
    DCHECK_LE(num_players, max_num_players_);
    std::size_t alloc_count = alloc_count_.fetch_add(1);
    if (alloc_count < batch_) {
      uint64_t increment = static_cast<uint64_t>(num_players) << 32 | 1;
      uint64_t offsets = offsets_.fetch_add(increment);
      uint32_t player_offset = offsets >> 32;
      uint32_t shared_offset = offsets;
      DCHECK_LE((std::size_t)shared_offset + 1, batch_);
      DCHECK_LE((std::size_t)(player_offset + num_players),
                batch_ * max_num_players_);
      if (order != -1 && max_num_players_ == 1) {
        player_offset = shared_offset = order;
      }
      std::vector<Array> state;
      state.reserve(arrays_.size());
      for (std::size_t i = 0; i < arrays_.size(); ++i) {
        const Array& a = arrays_[i];
        if (is_player_state_[i]) {
          state.emplace_back(
              a.Slice(player_offset, player_offset + num_players));
        } else {
          state.emplace_back(a[shared_offset]);
        }
      }
      return WritableSlice{.arr = std::move(state),
                          .done_write = [this]() { Done(); }};
    }
    DLOG(INFO) << "Allocation failed, continue to the next block of memory";
    throw std::runtime_error("StateBuffer allocation failed - buffer full");
  }

  [[nodiscard]] std::pair<uint32_t, uint32_t> Offsets() const {
    uint32_t player_offset = offsets_ >> 32;
    uint32_t shared_offset = offsets_;
    return {player_offset, shared_offset};
  }

  void Done(std::size_t num = 1) {
    std::size_t done_count = done_count_.fetch_add(num);
    if (done_count + num == batch_) {
      sem_.signal();
    }
  }

  std::vector<Array> Wait(std::size_t additional_done_count = 0) {
    if (additional_done_count > 0) {
      Done(additional_done_count);
    }
    while (!sem_.wait()) {
    }
    uint64_t offsets = offsets_;
    uint32_t player_offset = (offsets >> 32);
    uint32_t shared_offset = offsets;
    DCHECK_EQ((std::size_t)shared_offset, batch_ - additional_done_count);
    std::vector<Array> ret;
    ret.reserve(arrays_.size());
    for (std::size_t i = 0; i < arrays_.size(); ++i) {
      const Array& a = arrays_[i];
      if (is_player_state_[i]) {
        ret.emplace_back(a.Truncate(player_offset));
      } else {
        ret.emplace_back(a.Truncate(shared_offset));
      }
    }
    return ret;
  }
};

#endif  // LITEPOOL_CORE_STATE_BUFFER_H_
