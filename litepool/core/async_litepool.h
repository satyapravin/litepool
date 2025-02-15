#ifndef LITEPOOL_CORE_ASYNC_LITEPOOL_H_
#define LITEPOOL_CORE_ASYNC_LITEPOOL_H_

#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>
#include <condition_variable>

#include <threadpool/ThreadPool.h>
#include "litepool/core/action_buffer_queue.h"
#include "litepool/core/array.h"
#include "litepool/core/litepool.h"
#include "litepool/core/spec.h"
#include "litepool/core/state_buffer_queue.h"

template <typename Env>
class AsyncLitePool : public LitePool<typename Env::Spec> {
protected:
    std::size_t num_envs_;
    std::size_t batch_;
    std::size_t max_num_players_;
    std::size_t num_threads_;
    bool is_sync_;
    std::atomic<bool> stop_;
    std::atomic<std::size_t> stepping_env_num_;
    std::vector<std::thread> workers_;
    std::unique_ptr<ActionBufferQueue> action_buffer_queue_;
    std::unique_ptr<StateBufferQueue> state_buffer_queue_;
    std::vector<std::unique_ptr<Env>> envs_;
    std::vector<std::mutex> env_mutexes_;
    std::mutex initialization_mutex_;
    std::condition_variable initialization_cv_;
    std::atomic<size_t> initialized_envs_{0};
    std::chrono::duration<double> dur_send_{}, dur_recv_{};

public:
    using Spec = typename Env::Spec;
    using Action = typename Env::Action;
    using State = typename Env::State;
    using ActionSlice = typename ActionBufferQueue::ActionSlice;

    explicit AsyncLitePool(const Spec& spec)
        : LitePool<Spec>(spec)
        , num_envs_(spec.config["num_envs"_])
        , batch_(spec.config["batch_size"_] <= 0 ? num_envs_ : spec.config["batch_size"_])
        , max_num_players_(spec.config["max_num_players"_])
        , num_threads_(spec.config["num_threads"_])
        , is_sync_(batch_ == num_envs_ && max_num_players_ == 1)
        , stop_(false)
        , stepping_env_num_(0)
        , action_buffer_queue_(std::make_unique<ActionBufferQueue>(num_envs_))
        , state_buffer_queue_(std::make_unique<StateBufferQueue>(
              batch_, num_envs_, max_num_players_,
              spec.state_spec.template AllValues<ShapeSpec>()))
        , envs_(num_envs_)
        , env_mutexes_(num_envs_) {

        // Initialize number of threads
        if (num_threads_ == 0) {
            num_threads_ = std::min<std::size_t>(batch_, std::thread::hardware_concurrency());
        }

        // Initialize environments sequentially with proper synchronization
        for (std::size_t i = 0; i < num_envs_; ++i) {
            std::lock_guard<std::mutex> lock(env_mutexes_[i]);
            try {
                envs_[i] = std::make_unique<Env>(spec, i);
                initialized_envs_++;
            } catch (const std::exception& e) {
                LOG(ERROR) << "Failed to initialize environment " << i << ": " << e.what();
                throw;
            }
        }

        // Wait for all environments to be initialized
        {
            std::unique_lock<std::mutex> lock(initialization_mutex_);
            initialization_cv_.wait(lock, [this] {
                return initialized_envs_ == num_envs_;
            });
        }

        // Start worker threads
        for (std::size_t i = 0; i < num_threads_; ++i) {
            workers_.emplace_back([this] {
                while (!stop_) {
                    try {
                        ActionSlice raw_action = action_buffer_queue_->Dequeue();
                        if (stop_) break;

                        int env_id = raw_action.env_id;
                        if (env_id < 0 || static_cast<size_t>(env_id) >= num_envs_) {
                            LOG(ERROR) << "Invalid environment ID: " << env_id;
                            continue;
                        }

                        std::unique_lock<std::mutex> lock(env_mutexes_[env_id]);
                        if (!envs_[env_id]) {
                            LOG(ERROR) << "Environment " << env_id << " not initialized";
                            continue;
                        }

                        bool reset = raw_action.force_reset || envs_[env_id]->IsDone();
                        envs_[env_id]->EnvStep(state_buffer_queue_.get(),
                                             raw_action.order, reset);
                    } catch (const std::exception& e) {
                        if (!stop_) {
                            LOG(ERROR) << "Worker thread error: " << e.what();
                        }
                    }
                }
            });
        }

        // Set thread affinity if requested
        if (spec.config["thread_affinity_offset"_] >= 0) {
            SetThreadAffinity(spec.config["thread_affinity_offset"_]);
        }
    }

    ~AsyncLitePool() override {
        stop_ = true;

        // Signal all workers to stop
        std::vector<ActionSlice> empty_actions(workers_.size());
        action_buffer_queue_->EnqueueBulk(empty_actions);

        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

protected:
    void SetThreadAffinity(size_t thread_affinity_offset) {
        size_t processor_count = std::thread::hardware_concurrency();
        for (std::size_t tid = 0; tid < num_threads_; ++tid) {
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            size_t cid = (thread_affinity_offset + tid) % processor_count;
            CPU_SET(cid, &cpuset);
            pthread_setaffinity_np(workers_[tid].native_handle(),
                                 sizeof(cpu_set_t), &cpuset);
        }
    }

    template <typename V>
    void SendImpl(V&& action) {
        int* env_id = static_cast<int*>(action[0].Data());
        int shared_offset = action[0].Shape(0);
        std::vector<ActionSlice> actions;
        actions.reserve(shared_offset);

        auto action_batch = std::make_shared<std::vector<Array>>(std::forward<V>(action));

        for (int i = 0; i < shared_offset; ++i) {
            int eid = env_id[i];
            if (eid < 0 || static_cast<size_t>(eid) >= num_envs_) {
                throw std::runtime_error("Invalid environment ID: " + std::to_string(eid));
            }

            {
                std::lock_guard<std::mutex> lock(env_mutexes_[eid]);
                if (!envs_[eid]) {
                    throw std::runtime_error("Environment not initialized: " + std::to_string(eid));
                }
                envs_[eid]->SetAction(action_batch, i);
            }

            actions.emplace_back(ActionSlice{
                .env_id = eid,
                .order = is_sync_ ? i : -1,
                .force_reset = false,
            });
        }

        if (is_sync_) {
            stepping_env_num_ += shared_offset;
        }
        action_buffer_queue_->EnqueueBulk(actions);
    }

public:
    void Send(const Action& action) {
        SendImpl(action.template AllValues<Array>());
    }

    void Send(const std::vector<Array>& action) override {
        SendImpl(action);
    }

    void Send(std::vector<Array>&& action) override {
        SendImpl(std::move(action));
    }

    std::vector<Array> Recv() override {
        int additional_wait = 0;
        if (is_sync_ && stepping_env_num_ < batch_) {
            additional_wait = batch_ - stepping_env_num_;
        }
        auto ret = state_buffer_queue_->Wait(additional_wait);
        if (is_sync_) {
            stepping_env_num_ -= ret[0].Shape(0);
        }
        return ret;
    }

    void Reset(const Array& env_ids) override {
        TArray<int> tenv_ids(env_ids);
        int shared_offset = tenv_ids.Shape(0);
        std::vector<ActionSlice> actions;
        actions.reserve(shared_offset);

        for (int i = 0; i < shared_offset; ++i) {
            int eid = tenv_ids[i];
            if (eid < 0 || static_cast<size_t>(eid) >= num_envs_) {
                throw std::runtime_error("Invalid environment ID in reset: " + std::to_string(eid));
            }

            actions.emplace_back(ActionSlice{
                .env_id = eid,
                .order = is_sync_ ? i : -1,
                .force_reset = true,
            });
        }

        if (is_sync_) {
            stepping_env_num_ += shared_offset;
        }
        action_buffer_queue_->EnqueueBulk(actions);
    }
};

#endif  // LITEPOOL_CORE_ASYNC_LITEPOOL_H_
