#ifndef LITEPOOL_CORE_STATE_BUFFER_QUEUE_H_
#define LITEPOOL_CORE_STATE_BUFFER_QUEUE_H_

#include <algorithm>
#include <cstdint>
#include <memory>
#include <thread>
#include <utility>
#include <vector>
#include <mutex>

#include "litepool/core/array.h"
#include "litepool/core/spec.h"
#include "litepool/core/state_buffer.h"

class StateBufferQueue {
 protected:
  std::size_t batch_;
  std::size_t max_num_players_;
  std::vector<bool> is_player_state_;
  std::vector<ShapeSpec> specs_;
  std::size_t queue_size_;
  std::vector<std::unique_ptr<StateBuffer>> queue_;
  alignas(64) std::atomic<uint64_t> alloc_count_{0};
  alignas(64) std::atomic<uint64_t> done_ptr_{0};
  std::mutex mutex_;
  std::atomic<bool> shutdown_{false};
  std::atomic<size_t> active_allocations_{0};

 public:
  StateBufferQueue(std::size_t batch_env, std::size_t num_envs,
                   std::size_t max_num_players,
                   const std::vector<ShapeSpec>& specs)
      : batch_(batch_env),
        max_num_players_(max_num_players),
        is_player_state_(Transform(specs,
                                 [](const ShapeSpec& s) {
                                   return (!s.shape.empty() &&
                                         s.shape[0] == -1);
                                 })),
        specs_(Transform(specs,
                       [batch = batch_env, max_players = max_num_players](ShapeSpec s) {
                         if (!s.shape.empty() && s.shape[0] == -1) {
                           s.shape[0] = batch * max_players;
                           return s;
                         }
                         return s.Batch(batch);
                       })),
        queue_size_((num_envs / batch_env + 2) * 2),
        queue_(queue_size_) {
    
    for (auto& q : queue_) {
      q = std::make_unique<StateBuffer>(batch_, max_num_players_, specs_,
                                      is_player_state_);
    }
  }

  ~StateBufferQueue() {
    shutdown_.store(true, std::memory_order_release);
    
    // Wait for all active allocations to complete
    while (active_allocations_.load(std::memory_order_acquire) > 0) {
      std::this_thread::yield();
    }
    
    // Clear all buffers
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.clear();
  }

  StateBuffer::WritableSlice Allocate(std::size_t num_players, int order = -1) {
    if (shutdown_.load(std::memory_order_acquire)) {
      throw std::runtime_error("Queue is shutting down");
    }

    active_allocations_.fetch_add(1, std::memory_order_acq_rel);
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (shutdown_.load(std::memory_order_acquire)) {
      active_allocations_.fetch_sub(1, std::memory_order_release);
      throw std::runtime_error("Queue is shutting down");
    }

    std::size_t pos = alloc_count_.fetch_add(1, std::memory_order_acq_rel);
    std::size_t offset = (pos / batch_) % queue_size_;
    
    try {
      auto slice = queue_[offset]->Allocate(num_players, order);
      auto done_callback = [this, original_done = slice.done_write]() {
        original_done();
        active_allocations_.fetch_sub(1, std::memory_order_release);
      };
      return StateBuffer::WritableSlice{slice.arr, done_callback};
    } catch (const std::runtime_error&) {
      if (!shutdown_.load(std::memory_order_acquire)) {
        auto newbuf = std::make_unique<StateBuffer>(
            batch_, max_num_players_, specs_, is_player_state_);
        std::swap(queue_[offset], newbuf);
        auto slice = queue_[offset]->Allocate(num_players, order);
        auto done_callback = [this, original_done = slice.done_write]() {
          original_done();
          active_allocations_.fetch_sub(1, std::memory_order_release);
        };
        return StateBuffer::WritableSlice{slice.arr, done_callback};
      }
      active_allocations_.fetch_sub(1, std::memory_order_release);
      throw;
    }
  }

  std::vector<Array> Wait(std::size_t additional_done_count = 0) {
    if (shutdown_.load(std::memory_order_acquire)) {
      throw std::runtime_error("Queue is shutting down");
    }

    std::lock_guard<std::mutex> lock(mutex_);
    std::size_t pos = done_ptr_.fetch_add(1, std::memory_order_acq_rel);
    std::size_t offset = pos % queue_size_;

    auto arr = queue_[offset]->Wait(additional_done_count);
    
    if (additional_done_count > 0) {
      alloc_count_.fetch_add(additional_done_count, std::memory_order_release);
    }

    if (!shutdown_.load(std::memory_order_acquire)) {
      auto newbuf = std::make_unique<StateBuffer>(
          batch_, max_num_players_, specs_, is_player_state_);
      std::swap(queue_[offset], newbuf);
    }

    return arr;
  }
};

#endif  // LITEPOOL_CORE_STATE_BUFFER_QUEUE_H_
