// state_buffer_queue.h
#ifndef LITEPOOL_CORE_STATE_BUFFER_QUEUE_H_
#define LITEPOOL_CORE_STATE_BUFFER_QUEUE_H_

#include <algorithm>
#include <cstdint>
#include <list>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include "litepool/core/array.h"
#include "litepool/core/circular_buffer.h"
#include "litepool/core/spec.h"
#include "litepool/core/state_buffer.h"
#include <concurrentqueue/moodycamel/lightweightsemaphore.h>

class StateBufferQueue {
 protected:
  std::size_t batch_;
  std::size_t max_num_players_;
  std::vector<bool> is_player_state_;
  std::vector<ShapeSpec> specs_;
  std::size_t queue_size_;
  std::vector<std::unique_ptr<StateBuffer>> queue_;
  std::atomic<uint64_t> alloc_count_, done_ptr_;

  CircularBuffer<std::unique_ptr<StateBuffer>> stock_buffer_;
  std::vector<std::thread> create_buffer_thread_;
  std::atomic<bool> quit_;

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
                       [=](ShapeSpec s) {
                         if (!s.shape.empty() && s.shape[0] == -1) {
                           s.shape[0] = batch_ * max_num_players_;
                           return s;
                         }
                         return s.Batch(batch_);
                       })),
        queue_size_((num_envs / batch_env + 2) * 2),
        queue_(queue_size_),
        alloc_count_(0),
        done_ptr_(0),
        stock_buffer_((num_envs / batch_env + 2) * 2),
        quit_(false) {
    for (auto& q : queue_) {
      q = std::make_unique<StateBuffer>(batch_, max_num_players_, specs_,
                                      is_player_state_);
    }
    std::size_t processor_count = std::thread::hardware_concurrency();
    std::size_t create_buffer_thread_num = std::max(1UL, processor_count / 64);
    for (std::size_t i = 0; i < create_buffer_thread_num; ++i) {
      create_buffer_thread_.emplace_back(std::thread([&]() {
        while (true) {
          stock_buffer_.Put(std::make_unique<StateBuffer>(
              batch_, max_num_players_, specs_, is_player_state_));
          if (quit_) {
            break;
          }
        }
      }));
    }
  }

  ~StateBufferQueue() {
    quit_ = true;
    for (std::size_t i = 0; i < create_buffer_thread_.size(); ++i) {
      stock_buffer_.Get();
    }
    for (auto& t : create_buffer_thread_) {
      t.join();
    }
  }

  StateBuffer::WritableSlice Allocate(std::size_t num_players, int order = -1) {
    std::size_t pos = alloc_count_.fetch_add(1);
    std::size_t offset = (pos / batch_) % queue_size_;
    try {
      return queue_[offset]->Allocate(num_players, order);
    } catch (const std::runtime_error& e) {
      auto newbuf = stock_buffer_.Get();
      if (newbuf) {
        std::swap(queue_[offset], newbuf);
        return queue_[offset]->Allocate(num_players, order);
      }
      throw;
    }
  }

  StateBuffer* AllocateRaw(std::size_t num_players, int order = -1) {
    std::size_t pos = alloc_count_.fetch_add(1);
    std::size_t offset = (pos / batch_) % queue_size_;
    return queue_[offset].get();
  }

  std::vector<Array> Wait(std::size_t additional_done_count = 0) {
    std::unique_ptr<StateBuffer> newbuf = stock_buffer_.Get();
    std::size_t pos = done_ptr_.fetch_add(1);
    std::size_t offset = pos % queue_size_;
    auto arr = queue_[offset]->Wait(additional_done_count);
    if (additional_done_count > 0) {
      alloc_count_.fetch_add(additional_done_count);
    }
    std::swap(queue_[offset], newbuf);
    return arr;
  }
};

#endif  // LITEPOOL_CORE_STATE_BUFFER_QUEUE_H_
