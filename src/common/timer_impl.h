#ifndef MINITCP_SRC_COMMON_TIMER_IMPL_H_
#define MINITCP_SRC_COMMON_TIMER_IMPL_H_

#include <chrono>
#include <condition_variable>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

#include "constant.h"
#include "logging.h"

namespace minitcp {
using timestamp_t = std::chrono::time_point<std::chrono::high_resolution_clock>;

class TimerHandler {
   public:
    TimerHandler(std::size_t handler_id, std::function<void()> handler_callback)
        : handler_id_(handler_id), handler_callback_(handler_callback) {}
    ~TimerHandler() = default;
    std::size_t GetHandlerId() const { return handler_id_; }
    void DoTask() { handler_callback_(); }

   private:
    std::size_t handler_id_;
    std::function<void()> handler_callback_;
};

class TimerHandlerFactory {
   public:
    TimerHandlerFactory() = default;
    ~TimerHandlerFactory() = default;

    // no copy and move
    TimerHandlerFactory& operator=(const TimerHandlerFactory&) = delete;
    TimerHandlerFactory(const TimerHandlerFactory&) = delete;
    TimerHandlerFactory& operator=(TimerHandlerFactory&&) = delete;
    TimerHandlerFactory(TimerHandlerFactory&&) = delete;

    template <class F, class... Args>
    std::shared_ptr<class TimerHandler> CreateTimerHandler(
        F&& function, Args&&... arguments) {
        std::size_t handler_id = GetNextId();
        auto task = std::bind(std::forward<F>(function),
                              std::forward<Args>(arguments)...);
        std::function<void()> callback = [task]() { task(); };
        return std::shared_ptr<class TimerHandler>(
            new TimerHandler(handler_id, callback));
    }

    std::size_t GetNextId() {
        std::size_t id = 0;
        std::scoped_lock lock(factory_mutex_);
        // Loopback when overflow.
        // Reserve the first 10 handler id for service such as arp updating, ip
        // routing etc.
        if (next_handler_id_ == std::numeric_limits<std::size_t>::max()) {
            MINITCP_LOG(INFO)
                << "TimerHandlerFactory : handler id loopback." << std::endl;
            id = next_handler_id_;
            next_handler_id_ = kReservedHandlerId;
        } else {
            MINITCP_LOG(DEBUG) << "TimerHandlerFactory : next id is "
                               << next_handler_id_ << std::endl;
            id = next_handler_id_++;
        }
        return id;
    }

   private:
    std::mutex factory_mutex_;
    std::size_t next_handler_id_{0};
};

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
    std::optional<std::size_t> SetTimeoutAfter(int milliseconds, F&& function,
                                               Args&&... arguments) {
        std::size_t handler_id;
        auto expire_time = std::chrono::milliseconds(milliseconds) +
                           std::chrono::high_resolution_clock::now();
        return SetTimeoutAt(expire_time, function, arguments...);
    }

    tempate<class F, class... Args> std::optional<std::size_t> SetTimeoutAt(
        timestamp_t timestamp, F&& function, Args&&... arguments) {
        std::size_t handler_id;
        MINITCP_LOG(INFO) << "Timer: add timeout at "
                          << timestamp.time_since_epoch().count() << std::endl;
        {
            std::scoped_lock lock(timer_mutex_);
            auto new_handler =
                factory_.CreateTimerHandler(function, arguments...);
            handler_id = new_handler->GetHandlerId();
            if (this->stop_) {
                MINITCP_LOG(ERROR)
                    << "Timer: adding task into a stopped timer!" << std::endl;
                return {};
            }
            auto schedule_iter =
                schedule_.insert(std::make_pair(timestamp, new_handler));
            events_.insert(std::make_pair(handler_id, schedule_iter));
        }
        timer_cv_.notify_all();
        return handler_id;
    }

    int UnSetTimeout(std::size_t handler_id) {
        std::scoped_lock lock(timer_mutex_);
        auto event_iter = events_.find(handler_id);
        if (event_iter == events_.end()) {
            MINITCP_LOG(ERROR)
                << "Timer: removing task doesn't exist." << std::endl;
            return 1;
        }
        auto schedule_iter = event_iter->second;
        schedule_.erase(schedule_iter);
        events_.erase(event_iter);
        return 0;
    }

    void Start() {
        timer_worker_ = std::thread([this]() {
            while (true) {
                std::shared_ptr<class TimerHandler> handler;
                {
                    std::unique_lock<std::mutex> lock(timer_mutex_);
                    this->timer_cv_.wait(lock, [this]() {
                        return this->stop_ || !this->schedule_.empty();
                    });
                    if (this->stop_ && this->schedule_.empty()) {
                        return;
                    }
                    auto schedule_iter = schedule_.begin();
                    auto expire_time = schedule_iter->first;
                    handler = schedule_iter->second;
                    if (std::chrono::high_resolution_clock::now() <
                        expire_time) {
                        this->timer_cv_.wait_until(lock, schedule_iter->first);
                    }
                    // May be cancelled when sleeping, to see if it
                    // is still in the event_queue.
                    if (events_.find(handler->GetHandlerId()) !=
                        events_.end()) {
                        schedule_.erase(schedule_iter);
                        events_.erase(handler->GetHandlerId());
                    } else {
                        continue;
                    }
                }
                handler->DoTask();
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
    class TimerHandlerFactory factory_;
    std::mutex timer_mutex_;
    std::condition_variable timer_cv_;
    std::multimap<timestamp_t, std::shared_ptr<class TimerHandler>> schedule_;
    std::multimap<std::size_t,
                  std::multimap<timestamp_t,
                                std::shared_ptr<class TimerHandler>>::iterator>
        events_;
    std::thread timer_worker_;
};

}  // namespace minitcp

#endif  // ! MINITCP_SRC_COMMON_TIMER_IMPL_H_