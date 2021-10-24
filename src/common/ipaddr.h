#ifndef MINITCP_SRC_COMMON_IPADDR_H_
#define MINITCP_SRC_COMMON_IPADDR_H_

#include <arpa/inet.h>

#include <iostream>
#include <ostream>

#include "logging.h"
#include "types.h"

namespace minitcp {
class IpAddress {
   public:
    IpAddress() = default;
    ~IpAddress() = default;

    explicit IpAddress(const char* address) {
        int status = inet_aton(address, &ip_addr_);
        if (status == 0) {
            MINITCP_LOG(ERROR)
                << "IpAddress : inet_aton function fail." << std::endl;
        }
    }

    explicit IpAddress(const std::string& address) {
        int status = inet_aton(address.c_str(), &ip_addr_);
        if (status == 0) {
            MINITCP_LOG(ERROR)
                << "IpAddress : inet_aton function fail." << std::endl;
        }
    }

    explicit IpAddress(const ip_t& address) : ip_addr_(address) {}

    bool operator==(const IpAddress& other) {
        return ip_addr_.s_addr == other.ip_addr_.s_addr;
    }

    bool IsBroadcast() { return ip_addr_.s_addr == (unsigned int)-1; }

    void SetInAddr(const ip_t& other) { ip_addr_ = other; }
    const ip_t& GetInAddr() const { return ip_addr_; }

    const ip_t* GetPointer() const { return &ip_addr_; }

    friend std::ostream& operator<<(std::ostream& os,
                                    const IpAddress& address) {
        os << inet_ntoa(address.ip_addr_);
        return os;
    }

   private:
    ip_t ip_addr_{0};
};
}  // namespace minitcp

#endif  // ! MINITCP_SRC_COMMON_IPADDR_H_