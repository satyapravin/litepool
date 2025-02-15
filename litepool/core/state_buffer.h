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
        if (num_players > max_num_players_) {
            throw std::runtime_error("Number of players exceeds maximum");
        }

        std::size_t alloc_count = alloc_count_.fetch_add(1, std::memory_order_acq_rel);
        if (alloc_count >= batch_) {
            alloc_count_.fetch_sub(1, std::memory_order_release);
            throw std::runtime_error("StateBuffer allocation failed - buffer full");
        }

        std::lock_guard<std::mutex> lock(buffer_mutex_);
        uint64_t increment = static_cast<uint64_t>(num_players) << 32 | 1;
        uint64_t offsets = offsets_.fetch_add(increment, std::memory_order_acq_rel);
        uint32_t player_offset = offsets >> 32;
        uint32_t shared_offset = offsets & 0xFFFFFFFF;

        if (shared_offset >= batch_ || player_offset + num_players > batch_ * max_num_players_) {
            throw std::runtime_error("Buffer overflow detected");
        }

        if (order != -1 && max_num_players_ == 1) {
            player_offset = shared_offset = order;
        }

        std::vector<Array> state;
        state.reserve(arrays_.size());

        for (std::size_t i = 0; i < arrays_.size(); ++i) {
            if (is_player_state_[i]) {
                state.emplace_back(arrays_[i].Slice(player_offset, player_offset + num_players));
            } else {
                state.emplace_back(arrays_[i][shared_offset]);
            }
        }

        return WritableSlice{
            .arr = std::move(state),
            .done_write = [this]() { Done(); }
        };
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
