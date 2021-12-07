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
#include "types.h"

namespace minitcp {

class TimerHandler;
using handler_t = class TimerHandler*;

class TimerHandler {
 public:
  TimerHandler(bool is_persist) : is_persist_(is_persist) {}
  ~TimerHandler() = default;
  bool IsPersist() const { return is_persist_; }
  void RegisterCallback(std::function<void()> callback) {
    handler_callback_ = std::move(callback);
  }
  void RunTask() { handler_callback_(); }

 private:
  bool is_persist_;
  std::function<void()> handler_callback_;
};

class Timer {
 public:
  static class Timer& GetInstance() {
    static std::once_flag timer_once_flag_;
    std::call_once(timer_once_flag_, [&]() { timer_singleton_ = new Timer(); });
    return *timer_singleton_;
  }
  // no copy and no move
  Timer(const Timer&) = delete;
  Timer& operator=(const Timer&) = delete;
  Timer(Timer&&) = delete;
  Timer& operator=(Timer&&) = delete;

  std::optional<handler_t> SetTimeoutAfter(int milliseconds,
                                           handler_t handler) {
    auto expire_time = std::chrono::milliseconds(milliseconds) +
                       std::chrono::high_resolution_clock::now();
    return SetTimeoutAt(expire_time, handler);
  }

  std::optional<handler_t> SetTimeoutAt(timestamp_t timestamp,
                                        handler_t handler) {
    {
      std::scoped_lock lock(timer_mutex_);
      if (this->stop_) {
        return {};
      }
      auto schedule_iter = schedule_.insert(std::make_pair(timestamp, handler));
      events_.insert(std::make_pair(handler, schedule_iter));
    }
    timer_cv_.notify_all();
    return handler;
  }

  int UnSetTimeout(handler_t handler_id) {
    std::scoped_lock lock(timer_mutex_);
    auto event_iter = events_.find(handler_id);
    if (event_iter == events_.end()) {
      return 1;
    }
    auto schedule_iter = event_iter->second;

    // delete handler_id;
    delete handler_id;
    schedule_.erase(schedule_iter);
    events_.erase(event_iter);

    return 0;
  }

  void Start() {
    timer_worker_ = std::thread([this]() {
      while (true) {
        handler_t handler;
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
          if (std::chrono::high_resolution_clock::now() < expire_time) {
            this->timer_cv_.wait_until(lock, schedule_iter->first);
          }
          // May be cancelled when sleeping, to see if it
          // is still in the event_queue.
          if (events_.find(handler) != events_.end()) {
            schedule_.erase(schedule_iter);
            events_.erase(handler);
          } else {
            continue;
          }
        }
        handler->RunTask();
        if (!handler->IsPersist()) {
          MINITCP_LOG(INFO) << "timer thread : deleting timer "
                               "handler with handler id="
                            << handler << std::endl;
          delete handler;
        }
      }
    });
    // timer_worker_.detach();
  }

  void Stop() {
    {
      std::scoped_lock lock(timer_mutex_);
      stop_ = true;
    }
    timer_cv_.notify_all();
    timer_worker_.join();
  }

 private:
  static class Timer* timer_singleton_;

  Timer() = default;
  ~Timer() { Stop(); }
  bool stop_{0};
  std::mutex timer_mutex_;
  std::condition_variable timer_cv_;
  std::multimap<timestamp_t, handler_t> schedule_;
  std::multimap<handler_t, std::multimap<timestamp_t, handler_t>::iterator>
      events_;
  std::thread timer_worker_;
};

}  // namespace minitcp

#endif  // ! MINITCP_SRC_COMMON_TIMER_IMPL_H_