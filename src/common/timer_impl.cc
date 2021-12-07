#include "timer_impl.h"

namespace minitcp {
Timer* Timer::timer_singleton_ = nullptr;

class Timer& Timer::GetInstance() {
  static std::once_flag timer_once_flag_;
  std::call_once(timer_once_flag_, [&]() { timer_singleton_ = new Timer(); });
  return *timer_singleton_;
}

}  // namespace minitcp