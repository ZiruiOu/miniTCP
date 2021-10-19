#ifndef MINITCP_SRC_COMMON_TIMER_H_
#define MINITCP_SRC_COMMON_TIMER_H_

#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <thread>

#include "logging.h"

namespace minitcp {
using timestamp_t = std::chrono::time_point<std::chrono::high_resolution_clock>;

class Timer {
   public:
    Timer() = default;
    ~Timer() { Stop(); }

    // no copy and no move
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
    Timer(Timer&&) = delete;
    Timer& operator=(Timer&&) = delete;

    template <class F, class... Args>
    std::optional<timestamp_t> SetTimeout(int milliseconds, F&& function,
                                          Args&&... arguments) {
        auto expire_time = std::chrono::milliseconds(milliseconds) +
                           std::chrono::high_resolution_clock::now();
        auto task = std::bind(std::forward<F>(function),
                              std::forward<Args>(arguments)...);
        {
            std::scoped_lock(timer_mutex_);
            if (this->stop_) {
                MINITCP_LOG(ERROR)
                    << "Timer: adding task into a stopped timer!" << std::endl;
                return {};
            }
            events_.insert(std::make_pair(expire_time, task));
        }
        timer_cv_.notify_all();
        return expire_time;
    }

    int UnSetTimeout(timestamp_t index) {
        std::scoped_lock lock(timer_mutex_);
        auto iterator = events_.find(index);
        if (iterator == events_.end()) {
            MINITCP_LOG(ERROR)
                << "Timer: removing task doesn't exist." << std::endl;
            return 1;
        }
        events_.erase(iterator);
        return 0;
    }

    void Start() {
        timer_worker_ = std::thread([this]() {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(timer_mutex_);
                    this->timer_cv_.wait(lock, [this]() {
                        return this->stop_ || !this->events_.empty();
                    });
                    if (this->stop_ && this->events_.empty()) {
                        return;
                    }
                    auto iterator = events_.begin();
                    // TODO : Might be a good option to implement in
                    // std::this_thread::sleep_for(XXX)
                    if (std::chrono::high_resolution_clock::now() <
                        iterator->first) {
                        this->timer_cv_.wait_until(lock, iterator->first);
                    }
                    task = iterator->second;
                    events_.erase(iterator);
                }
                task();
            }
        });
        timer_worker_.detach();
    }

    void Stop() {
        {
            std::scoped_lock lock(timer_mutex_);
            stop_ = true;
        }
        timer_cv_.notify_all();
    }

   private:
    bool stop_{0};
    std::mutex timer_mutex_;
    std::condition_variable timer_cv_;
    std::multimap<timestamp_t, std::function<void()>> events_;
    std::thread timer_worker_;
};

}  // namespace minitcp

#endif  // ! MINITCP_SRC_COMMON_TIMER_H_