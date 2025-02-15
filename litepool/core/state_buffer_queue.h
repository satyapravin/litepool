#ifndef LITEPOOL_CORE_STATE_BUFFER_QUEUE_H_
#define LITEPOOL_CORE_STATE_BUFFER_QUEUE_H_

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <vector>

#include "litepool/core/array.h"
#include "litepool/core/circular_buffer.h"
#include "litepool/core/spec.h"

class StateBufferQueue {
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

public:
    StateBufferQueue(size_t batch_size,
                    size_t num_envs,
                    size_t max_num_players,
                    const std::vector<ShapeSpec>& specs)
        : batch_size_(batch_size)
        , num_envs_(num_envs)
        , max_num_players_(max_num_players)
        , capacity_(batch_size * 2) {

        // Initialize arrays with proper dimensions
        arrays_.reserve(specs.size());
        buffer_sizes_.reserve(specs.size());

        for (const auto& spec : specs) {
            std::vector<size_t> shape = spec.Shape();
            // Adjust first dimension for batch size
            if (!shape.empty()) {
                shape[0] = batch_size_;
            }
            arrays_.emplace_back(ShapeSpec(spec.element_size, shape));
            buffer_sizes_.push_back(shape.empty() ? 0 : shape[0]);
        }

        buffer_ = std::vector<Array>(capacity_ * specs.size());
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        current_size_ = 0;
        head_ = 0;
        tail_ = 0;
    }

    void EnsureCapacity(size_t required_size) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (capacity_ < required_size) {
            ResizeBuffer(std::max(capacity_ * 2, required_size));
        }
    }

    size_t AllocateSlot() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (current_size_ >= capacity_) {
            ResizeBuffer(capacity_ * 2);
        }
        size_t slot = tail_;
        tail_ = (tail_ + 1) % capacity_;
        current_size_++;
        return slot;
    }

    std::vector<Array> Wait(int timeout_ms = -1) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (timeout_ms < 0) {
            cv_.wait(lock, [this] { return current_size_ > 0; });
        } else {
            if (!cv_.wait_for(lock,
                std::chrono::milliseconds(timeout_ms),
                [this] { return current_size_ > 0; })) {
                return {};
            }
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

    void Set(size_t slot, const std::vector<Array>& state) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (slot >= capacity_) {
            throw std::runtime_error("Invalid slot index");
        }

        if (state.size() != arrays_.size()) {
            throw std::runtime_error("State size mismatch");
        }

        for (size_t i = 0; i < state.size(); ++i) {
            buffer_[slot * arrays_.size() + i] = state[i];
        }
        cv_.notify_one();
    }

protected:
    void ResizeBuffer(size_t new_capacity) {
        std::vector<Array> new_buffer(new_capacity * arrays_.size());

        // Copy existing data
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