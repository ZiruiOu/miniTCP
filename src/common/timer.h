#ifndef MINITCP_SRC_COMMON_TIMER_H_
#define MINITCP_SRC_COMMON_TIMER_H_

#include <functional>
#include <optional>

#include "timer_impl.h"

namespace minitcp {
#ifdef __cplusplus
extern "C" {
#endif  // ! __cplusplus

void timerStart();

void timerStop();

std::optional<handler_t> setTimerAfter(int milliseconds, handler_t handler);

int cancellTimer(handler_t handler);

#ifdef __cplusplus
}
#endif  // ! __cplusplus
}  // namespace minitcp

#endif  // ! MINITCP_SRC_COMMON_TIMER_H_