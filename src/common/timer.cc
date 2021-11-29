#include "timer.h"

#include <optional>

#include "../common/logging.h"
#include "timer_impl.h"

namespace minitcp {
void timerStart() {
  Timer& timer = Timer::GetInstance();
  MINITCP_LOG(INFO) << " Timer: timer start." << std::endl;
  timer.Start();
}

void timerStop() {
  Timer& timer = Timer::GetInstance();
  timer.Stop();
}

std::optional<handler_t> setTimerAfter(int milliseconds, handler_t handler) {
  Timer& timer = Timer::GetInstance();
  return timer.SetTimeoutAfter(milliseconds, handler);
}

int cancellTimer(handler_t handler) {
  Timer& timer = Timer::GetInstance();
  MINITCP_LOG(DEBUG) << "start calcelling timer." << std::endl;
  int status = timer.UnSetTimeout(handler);
  MINITCP_LOG(DEBUG) << "end of cancelling timer." << std::endl;
  return status;
}

}  // namespace minitcp