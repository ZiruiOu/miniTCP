#ifndef MINITCP_SRC_NETWORK_ROUTING_H_
#define MINITCP_SRC_NETWORK_ROUTING_H_

#include "../common/types.h"
#include "routing_impl.h"

namespace minitcp {
namespace network {
#ifdef __cplusplus
extern "C" {
#endif  // ! __cplusplus

void init();

int broadcastDVTable();

void receiveDVTableCallback(const void *buffer);

void timerRIPHandler();

#ifdef __cplusplus
}
#endif  // ! __cplusplus
}  // namespace network
}  // namespace minitcp

#endif  // ! MINITCP_SRC_NETWORK_ROUTING_H_