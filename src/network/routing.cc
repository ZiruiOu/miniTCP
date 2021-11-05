#include "routing.h"

#include <iostream>

#include "../common/logging.h"
#include "../common/timer.h"
#include "../ethernet/arp.h"
#include "../ethernet/device.h"

namespace minitcp {
namespace network {

static std::size_t routing_table_counter_ = 0;

void initRoutingTable() {
    class RoutingTable &routing_table = RoutingTable::GetInstance();
    routing_table.InitRoutingTable();
}

std::optional<ip_t> queryRoutingTable(ip_t dest_ip) {
    class RoutingTable &routing_table = RoutingTable::GetInstance();
    routing_table.Query(dest_ip);
}

int broadcastDVTable() {
    int num_device = ethernet::getDeviceNumber();
    mac_t broadcast_mac = mac_t{0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

    class RoutingTable &routing_table = RoutingTable::GetInstance();
    class ethernet::ArpTable &arp_table = ethernet::ArpTable::GetInstance();

    for (int i = 0; i < num_device; i++) {
        auto device = ethernet::getDevicePointer(i);
        auto entry = arp_table.QueryByDeviceId(i);
        ip_t nexthop_ip{0};
        if (entry) {
            nexthop_ip = entry->dest_ip;
        }
        std::pair<std::uint8_t *, int> dv_pair =
            routing_table.SerializeDistanceVector(device->GetIpAddress(),
                                                  nexthop_ip);
        device->SendFrame((const std::uint8_t *)dv_pair.first, dv_pair.second,
                          kEtherRouteType, &broadcast_mac);
        delete[] dv_pair.first;
    }
}

void receiveDVTableCallback(const void *buffer) {
    class RoutingTable &routing_table = RoutingTable::GetInstance();
    ip_t nexthop_ip = *(ip_t *)buffer;
    int dv_length = ntohl(*(int *)(buffer + 4));

    int modified =
        routing_table.UpdateByDistanceVector(nexthop_ip, buffer + 8, dv_length);
    if (modified) {
        broadcastDVTable();
    }
}

void timerRIPHandler() {
    handler_t rip_timer_handler = new TimerHandler(/*is_persistent = */ true);
    std::function<void()> rip_handler_wrapper = [rip_timer_handler]() {
        network::broadcastDVTable();
        network::RoutingTable &routing_table =
            network::RoutingTable::GetInstance();
        routing_table.Aging();
        setTimerAfter(/* milliseconds = */ 3000,
                      /* handler = */ rip_timer_handler);
    };
    rip_timer_handler->RegisterCallback(rip_handler_wrapper);
    setTimerAfter(/* milliseconds = */ 1000, /* handler = */ rip_timer_handler);
}

}  // namespace network
}  // namespace minitcp