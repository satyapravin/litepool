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
#include <mutex>

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
    alignas(64) std::atomic<uint64_t> offsets_{0};
    alignas(64) std::atomic<std::size_t> alloc_count_{0};
    alignas(64) std::atomic<std::size_t> done_count_{0};
    moodycamel::LightweightSemaphore sem_;
    std::mutex buffer_mutex_;

public:
    struct WritableSlice {
        std::vector<Array> arr;
        std::function<void()> done_write;
    };

    StateBuffer(std::size_t batch, std::size_t max_num_players,
                const std::vector<ShapeSpec>& specs,
                std::vector<bool> is_player_state)
        : batch_(batch)
        , max_num_players_(max_num_players)
        , is_player_state_(std::move(is_player_state)) {
        arrays_ = MakeArray(specs);
        done_count_.store(0, std::memory_order_release);
        offsets_.store(0, std::memory_order_release);
        alloc_count_.store(0, std::memory_order_release);
    }

    WritableSlice Allocate(std::size_t num_players, int order = -1) {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        
        if (num_players > max_num_players_) {
            throw std::runtime_error("Too many players requested");
        }

        uint64_t current_offsets = offsets_.load(std::memory_order_acquire);
        uint32_t player_offset = current_offsets >> 32;
        uint32_t shared_offset = current_offsets & 0xFFFFFFFF;

        if (shared_offset >= batch_) {
            throw std::runtime_error("Buffer full");
        }

        std::vector<Array> slice_arrays;
        slice_arrays.reserve(arrays_.size());

        for (std::size_t i = 0; i < arrays_.size(); ++i) {
            if (is_player_state_[i]) {
                slice_arrays.push_back(arrays_[i].Slice(player_offset, 
                    player_offset + num_players));
            } else {
                slice_arrays.push_back(arrays_[i].Slice(shared_offset, 
                    shared_offset + 1));
            }
        }

        if (order >= 0) {
            // Synchronous mode
            player_offset += num_players;
            shared_offset++;
        } else {
            // Asynchronous mode
            player_offset += num_players;
            shared_offset++;
        }

        uint64_t new_offsets = (static_cast<uint64_t>(player_offset) << 32) | 
                               static_cast<uint64_t>(shared_offset);
        offsets_.store(new_offsets, std::memory_order_release);
        alloc_count_.fetch_add(1, std::memory_order_release);

        auto done_write_fn = [this]() {
            Done(1);
        };

        return WritableSlice{std::move(slice_arrays), done_write_fn};
    }

    void Done(std::size_t num = 1) {
        std::size_t prev_done = done_count_.fetch_add(num, std::memory_order_acq_rel);
        if (prev_done + num >= batch_) {
            sem_.signal();
        }
    }

    std::vector<Array> Wait(std::size_t additional_done_count = 0) {
        if (additional_done_count > 0) {
            Done(additional_done_count);
        }

        while (!sem_.wait()) {
            std::this_thread::yield();
        }

        std::lock_guard<std::mutex> lock(buffer_mutex_);
        uint64_t current_offsets = offsets_.load(std::memory_order_acquire);
        uint32_t player_offset = current_offsets >> 32;
        uint32_t shared_offset = current_offsets & 0xFFFFFFFF;

        if (shared_offset != batch_ - additional_done_count) {
            throw std::runtime_error("State buffer synchronization error");
        }

        std::vector<Array> ret;
        ret.reserve(arrays_.size());

        for (std::size_t i = 0; i < arrays_.size(); ++i) {
            if (is_player_state_[i]) {
                ret.emplace_back(arrays_[i].Truncate(player_offset));
            } else {
                ret.emplace_back(arrays_[i].Truncate(shared_offset));
            }
        }

        // Reset counters
        offsets_.store(0, std::memory_order_release);
        alloc_count_.store(0, std::memory_order_release);
        done_count_.store(0, std::memory_order_release);

        return ret;
    }

    [[nodiscard]] std::pair<uint32_t, uint32_t> Offsets() const {
        uint64_t offs = offsets_.load(std::memory_order_acquire);
        return {
            static_cast<uint32_t>(offs >> 32),
            static_cast<uint32_t>(offs & 0xFFFFFFFF)
        };
    }
};

#endif  // LITEPOOL_CORE_STATE_BUFFER_H_
