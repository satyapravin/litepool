#ifndef LITEPOOL_CORE_STATE_BUFFER_QUEUE_H_
#define LITEPOOL_CORE_STATE_BUFFER_QUEUE_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <vector>

#include "litepool/core/array.h"
#include "litepool/core/circular_buffer.h"
#include "litepool/core/spec.h"

class StateBufferQueue {
public:
    struct Slice {
        std::vector<Array> arr;
        std::function<void()> done_write;
    };

protected:
    const size_t batch_size_;
    const size_t num_envs_;
    const size_t max_num_players_;
    std::vector<Array> arrays_;
    std::vector<size_t> buffer_sizes_;
    
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<Array> buffer_;
    size_t head_{0};
    size_t tail_{0};
    size_t capacity_;
    size_t current_size_{0};
    std::atomic<size_t> pending_writes_{0};

public:
    StateBufferQueue(size_t batch_size,
                    size_t num_envs,
                    size_t max_num_players,
                    const std::vector<ShapeSpec>& specs)
        : batch_size_(batch_size)
        , num_envs_(num_envs)
        , max_num_players_(max_num_players)
        , capacity_(batch_size * 2) {
        
        arrays_.reserve(specs.size());
        buffer_sizes_.reserve(specs.size());
        
        for (const auto& spec : specs) {
            std::vector<size_t> shape = spec.Shape();
            if (!shape.empty()) {
                if (shape[0] < 0) {
                    shape[0] = batch_size_ * max_num_players_;
                } else {
                    shape[0] = batch_size_;
                }
            }
            arrays_.emplace_back(ShapeSpec(spec.element_size, shape));
            buffer_sizes_.push_back(shape.empty() ? 0 : shape[0]);
        }
        
        buffer_ = std::vector<Array>(capacity_ * specs.size());
    }

    // Single-argument version (from test case)
    Slice Allocate(size_t num_players) {
        return Allocate(num_players, -1);
    }

    // Two-argument version
    Slice Allocate(size_t num_players, int order) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (current_size_ >= capacity_) {
            throw std::runtime_error("Buffer full");
        }
        if (num_players > max_num_players_) {
            throw std::runtime_error("Too many players");
        }

        size_t slot = tail_;
        tail_ = (tail_ + 1) % capacity_;
        current_size_++;
        pending_writes_++;

        std::vector<Array> slice_arrays;
        slice_arrays.reserve(arrays_.size());

        for (size_t i = 0; i < arrays_.size(); ++i) {
            const Array& src = arrays_[i];
            if (!src.Shape().empty()) {
                size_t first_dim = src.Shape(0) == batch_size_ * max_num_players_ ? 
                    num_players : 1;
                Array sliced = src.Slice(0, first_dim);
                buffer_[slot * arrays_.size() + i] = sliced;
                slice_arrays.push_back(std::move(sliced));
            } else {
                buffer_[slot * arrays_.size() + i] = src;
                slice_arrays.push_back(src);
            }
        }

        auto done_write_fn = [this]() {
            if (--pending_writes_ == 0) {
                cv_.notify_one();
            }
        };

        return Slice{std::move(slice_arrays), done_write_fn};
    }

    std::vector<Array> Wait(int additional_wait = 0) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        cv_.wait(lock, [this] { 
            return pending_writes_ == 0 && current_size_ > 0; 
        });

        if (additional_wait > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(additional_wait));
        }

        std::vector<Array> result;
        result.reserve(arrays_.size());

        size_t slot = head_;
        head_ = (head_ + 1) % capacity_;
        current_size_--;

        for (size_t i = 0; i < arrays_.size(); ++i) {
            result.push_back(buffer_[slot * arrays_.size() + i]);
        }

        return result;
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        current_size_ = 0;
        head_ = 0;
        tail_ = 0;
        pending_writes_ = 0;
    }

    void EnsureCapacity(size_t required_size) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (capacity_ < required_size) {
            ResizeBuffer(std::max(capacity_ * 2, required_size));
        }
    }

protected:
    void ResizeBuffer(size_t new_capacity) {
        std::vector<Array> new_buffer(new_capacity * arrays_.size());
        
        for (size_t i = 0; i < current_size_; ++i) {
            size_t old_idx = (head_ + i) % capacity_;
            size_t new_idx = i;
            for (size_t j = 0; j < arrays_.size(); ++j) {
                new_buffer[new_idx * arrays_.size() + j] = 
                    buffer_[old_idx * arrays_.size() + j];
            }
        }
        
        buffer_ = std::move(new_buffer);
        capacity_ = new_capacity;
        head_ = 0;
        tail_ = current_size_;
    }
};

#endif  // LITEPOOL_CORE_STATE_BUFFER_QUEUE_H_
