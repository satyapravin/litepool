#ifndef LITEPOOL_CORE_ACTION_BUFFER_QUEUE_H_
#define LITEPOOL_CORE_ACTION_BUFFER_QUEUE_H_

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>
#include "litepool/core/array.h"

class ActionBufferQueue {
public:
    struct ActionSlice {
        int env_id;
        int order;
        bool force_reset;
    };

protected:
    std::size_t queue_size_;
    std::vector<ActionSlice> queue_;
    std::size_t head_{0};
    std::size_t tail_{0}; 
    std::size_t size_{0};
    mutable std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;

public:
    explicit ActionBufferQueue(std::size_t num_envs)
        : queue_size_(num_envs * 2),
          queue_(queue_size_) {}

    void EnqueueBulk(const std::vector<ActionSlice>& actions) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        // Wait until there's enough space
        not_full_.wait(lock, [this, &actions]() {
            return size_ + actions.size() <= queue_size_;
        });

        // Add actions to queue
        for (const auto& action : actions) {
            queue_[tail_] = action;
            tail_ = (tail_ + 1) % queue_size_;
            size_++;
        }

        lock.unlock();
        not_empty_.notify_all();
    }

    ActionSlice Dequeue() {
        std::unique_lock<std::mutex> lock(mutex_);
        
        // Wait until there's at least one item
        not_empty_.wait(lock, [this]() {
            return size_ > 0;
        });

        ActionSlice result = queue_[head_];
        head_ = (head_ + 1) % queue_size_;
        size_--;

        lock.unlock();
        not_full_.notify_one();
        return result;
    }

    std::size_t SizeApprox() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_;
    }
};

#endif // LITEPOOL_CORE_ACTION_BUFFER_QUEUE_H_
