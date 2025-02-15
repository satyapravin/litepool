#ifndef LITEPOOL_CORE_STATE_BUFFER_QUEUE_H_
#define LITEPOOL_CORE_STATE_BUFFER_QUEUE_H_

#include <algorithm>
#include <cstdint>
#include <memory>
#include <thread>
#include <utility>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "litepool/core/array.h"
#include "litepool/core/spec.h"
#include "litepool/core/state_buffer.h"

class StateBufferQueue {
 protected:
  const std::size_t batch_;
  const std::size_t max_num_players_;
  const std::vector<bool> is_player_state_;
  const std::vector<ShapeSpec> specs_;
  const std::size_t queue_size_;
  std::vector<std::unique_ptr<StateBuffer>> queue_;
  alignas(64) std::atomic<uint64_t> alloc_count_{0};
  alignas(64) std::atomic<uint64_t> done_ptr_{0};
  mutable std::mutex mutex_;
  std::condition_variable cv_;
  std::atomic<bool> shutdown_{false};
  alignas(64) std::atomic<size_t> active_allocations_{0};

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
    {
      std::unique_lock<std::mutex> lock(mutex_);
      shutdown_.store(true, std::memory_order_release);
      cv_.notify_all();
    }
    
    // Wait for all active allocations to complete with timeout
    const auto start = std::chrono::steady_clock::now();
    while (active_allocations_.load(std::memory_order_acquire) > 0) {
      if (std::chrono::steady_clock::now() - start > std::chrono::seconds(5)) {
        break;  // Timeout after 5 seconds
      }
      std::this_thread::yield();
    }
    
    // Clear all buffers
    std::unique_lock<std::mutex> lock(mutex_);
    queue_.clear();
  }

  StateBuffer::WritableSlice Allocate(std::size_t num_players, int order = -1) {
    if (shutdown_.load(std::memory_order_acquire)) {
      throw std::runtime_error("Queue is shutting down");
    }

    active_allocations_.fetch_add(1, std::memory_order_acq_rel);
    std::unique_lock<std::mutex> lock(mutex_);
    
    if (shutdown_.load(std::memory_order_acquire)) {
      active_allocations_.fetch_sub(1, std::memory_order_release);
      throw std::runtime_error("Queue is shutting down");
    }

    std::size_t pos = alloc_count_.fetch_add(1, std::memory_order_acq_rel);
    std::size_t offset = (pos / batch_) % queue_size_;
    
    try {
      // Release lock while allocating buffer
      auto queue_ptr = queue_[offset].get();
      lock.unlock();
      
      auto slice = queue_ptr->Allocate(num_players, order);
      
      // Create a shared state for synchronization
      struct SharedState {
        std::mutex& mutex;
        std::condition_variable& cv;
        std::atomic<size_t>& active_allocations;
        bool notified{false};
      };
      
      auto shared_state = std::make_shared<SharedState>(
          SharedState{mutex_, cv_, active_allocations_});
      
      auto done_callback = [original_done = slice.done_write, shared_state]() {
        {
          std::lock_guard<std::mutex> lock(shared_state->mutex);
          original_done();
          shared_state->active_allocations.fetch_sub(1, std::memory_order_release);
          shared_state->notified = true;
        }
        shared_state->cv.notify_all();
      };
      
      return StateBuffer::WritableSlice{slice.arr, done_callback};
      
    } catch (const std::runtime_error& e) {
      if (!shutdown_.load(std::memory_order_acquire)) {
        lock.lock();
        auto newbuf = std::make_unique<StateBuffer>(
            batch_, max_num_players_, specs_, is_player_state_);
        std::swap(queue_[offset], newbuf);
        auto queue_ptr = queue_[offset].get();
        lock.unlock();
        
        auto slice = queue_ptr->Allocate(num_players, order);
        
        // Create a shared state for synchronization
        struct SharedState {
          std::mutex& mutex;
          std::condition_variable& cv;
          std::atomic<size_t>& active_allocations;
          bool notified{false};
        };
        
        auto shared_state = std::make_shared<SharedState>(
            SharedState{mutex_, cv_, active_allocations_});
        
        auto done_callback = [original_done = slice.done_write, shared_state]() {
          {
            std::lock_guard<std::mutex> lock(shared_state->mutex);
            original_done();
            shared_state->active_allocations.fetch_sub(1, std::memory_order_release);
            shared_state->notified = true;
          }
          shared_state->cv.notify_all();
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

    std::unique_lock<std::mutex> lock(mutex_);
    
    // Wait for data to be available or shutdown
    bool got_data = cv_.wait_for(lock, 
                                std::chrono::milliseconds(100),
                                [this]() {
        return shutdown_.load(std::memory_order_acquire) ||
               alloc_count_.load(std::memory_order_acquire) > 
               done_ptr_.load(std::memory_order_acquire);
    });

    if (!got_data) {
        if (shutdown_.load(std::memory_order_acquire)) {
            throw std::runtime_error("Queue is shutting down");
        }
        return {};  // Timeout
    }

    std::size_t pos = done_ptr_.fetch_add(1, std::memory_order_acq_rel);
    std::size_t offset = pos % queue_size_;

    // Validate queue access
    if (offset >= queue_.size()) {
        LOG(ERROR) << "Invalid queue offset: " << offset << " >= " << queue_.size();
        return {};
    }

    // Get buffer pointer before releasing lock
    auto buffer_ptr = queue_[offset].get();
    if (!buffer_ptr) {
        LOG(ERROR) << "Null buffer pointer at offset " << offset;
        return {};
    }

    lock.unlock();
    
    std::vector<Array> arr;
    try {
        arr = buffer_ptr->Wait(additional_done_count);
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error in buffer Wait: " << e.what();
        return {};
    }

    // Validate returned array
    if (arr.empty()) {
        LOG(WARNING) << "Empty array returned from buffer";
        return arr;
    }

    if (!arr[0].Data()) {
        LOG(ERROR) << "Invalid data in returned array";
        return {};
    }

    lock.lock();
    if (additional_done_count > 0) {
        alloc_count_.fetch_add(additional_done_count, std::memory_order_release);
    }

    if (!shutdown_.load(std::memory_order_acquire)) {
        try {
            auto newbuf = std::make_unique<StateBuffer>(
                batch_, max_num_players_, specs_, is_player_state_);
            std::swap(queue_[offset], newbuf);
        } catch (const std::exception& e) {
            LOG(ERROR) << "Failed to create new buffer: " << e.what();
            // Continue without throwing as the array data is still valid
        }
    }

    return arr;
}
};

#endif  // LITEPOOL_CORE_STATE_BUFFER_QUEUE_H_
