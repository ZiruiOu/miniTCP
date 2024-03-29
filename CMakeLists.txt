cmake_minimum_required(VERSION 3.14)

project(miniTCP)


set(CMAKE_POSITION_INDEPENDENT_CODE on)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_EXTENSION off)

# include("${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libpcap/FindPCAP.cmake")
# Add subdirectory 3rdparty
add_subdirectory(3rdparty)
# Add subdirectory src.
add_subdirectory(src)

# Fetch Googletest from gitee mirror
option(ENABLE_UNIT_TESTS on)

set(ENABLE_UNIT_TESTS on)
message(STATUS "Enabling unit test : ${ENABLE_UNIT_TESTS}")
if (ENABLE_UNIT_TESTS)
    add_executable(test_gtest "")
    target_sources(test_gtest
        PRIVATE
            test_gtest.cc
    )
    target_link_libraries(test_gtest
        PRIVATE
            gtest_main
            utilities
    )
    enable_testing()
    add_test(
        NAME test_gtest
        COMMAND $<TARGET_FILE:test_gtest>
    )
    add_executable(test_intrusive "")
    target_sources(test_intrusive
        PRIVATE
            ./src/common/intrusive_test.cc
    )
    target_link_libraries(test_intrusive
        PRIVATE
            gtest_main
            utilities
    )
    enable_testing()
    add_test(
        NAME test_intrusive
        COMMAND $<TARGET_FILE:test_gtest>
    )
endif()

add_executable(link_send src/application/link/link_send.cc)
target_link_libraries(link_send
    PRIVATE
        utilities
        ethernet
        ${PCAP_LIBRARY}
)
add_executable(link_recv src/application/link/link_recv.cc)
target_link_libraries(link_recv
    PRIVATE
        utilities
        ethernet
        ${PCAP_LIBRARY}
)
add_executable(link_app src/application/link/link_app.cc)
target_link_libraries(link_app
    PRIVATE
        utilities
        ethernet
        ${PCAP_LIBRARY}
)
add_executable(arp src/application/arp/arp.cc)
target_link_libraries(arp
    PRIVATE
        utilities
        ethernet
        network
        ${PCAP_LIBRARY}
)
add_executable(route src/application/network/route.cc)
target_link_libraries(route
    PRIVATE
        utilities
        ethernet
        network
        ${PCAP_LIBRARY}
)
add_executable(ip src/application/network/ip.cc)
target_link_libraries(ip
    PRIVATE
        utilities
        ethernet
        network
        ${PCAP_LIBRARY}
)

# link-time interposition for wrapper functions.
set(INTERPOSITION_FLAG "-Wl")
set(INTERPOSITION_FLAG "${INTERPOSITION_FLAG},--wrap,socket")
set(INTERPOSITION_FLAG "${INTERPOSITION_FLAG},--wrap,bind")
set(INTERPOSITION_FLAG "${INTERPOSITION_FLAG},--wrap,listen")
set(INTERPOSITION_FLAG "${INTERPOSITION_FLAG},--wrap,connect")
set(INTERPOSITION_FLAG "${INTERPOSITION_FLAG},--wrap,accept")
set(INTERPOSITION_FLAG "${INTERPOSITION_FLAG},--wrap,read")
set(INTERPOSITION_FLAG "${INTERPOSITION_FLAG},--wrap,write")
set(INTERPOSITION_FLAG "${INTERPOSITION_FLAG},--wrap,close")
set(INTERPOSITION_FLAG "${INTERPOSITION_FLAG},--wrap,getaddrinfo")
add_executable(router src/application/transport/router.c)
target_link_libraries(router
    PRIVATE
        transport
        ${INTERPOSITION_FLAG}
)

#checkpoint 7
add_executable(tcp_sender src/application/transport/tcp_sender.cc)
target_link_libraries(tcp_sender
    PRIVATE
        transport
        ${PCAP_LIBRARY}
        ${INTERPOSITION_FLAG}
)
add_executable(tcp_receiver src/application/transport/tcp_receiver.cc)
target_link_libraries(tcp_receiver
    PRIVATE
        transport
        ${PCAP_LIBRARY}
        ${INTERPOSITION_FLAG}
)

#checkpoint 8
add_executable(broken_send src/application/checkpoint/broken_send.c src/application/checkpoint/unp.c)
target_link_libraries(broken_send
    PRIVATE
        transport
        ${INTERPOSITION_FLAG}
)
add_executable(broken_recv src/application/checkpoint/broken_recv.c src/application/checkpoint/unp.c)
target_link_libraries(broken_recv
    PRIVATE
        transport
        ${INTERPOSITION_FLAG}
)

# checkpoint 9
add_executable(echo_server src/application/checkpoint/echo_server.c src/application/checkpoint/unp.c)
target_link_libraries(echo_server
    PRIVATE
        transport
        ${INTERPOSITION_FLAG}
)
add_executable(echo_client src/application/checkpoint/echo_client.c src/application/checkpoint/unp.c)
target_link_libraries(echo_client
    PRIVATE
        transport
        ${INTERPOSITION_FLAG}
)

#checkpoint 10
add_executable(perf_server src/application/checkpoint/perf_server.c src/application/checkpoint/unp.c)
target_link_libraries(perf_server
    PRIVATE
        transport
        ${INTERPOSITION_FLAG}
)
add_executable(perf_client src/application/checkpoint/perf_client.c src/application/checkpoint/unp.c)
target_link_libraries(perf_client
    PRIVATE
        transport
        ${INTERPOSITION_FLAG}
)

#challenge 5
add_executable(cong_server src/application/checkpoint/cong_server.c src/application/checkpoint/unp.c)
target_link_libraries(cong_server
    PRIVATE
        transport
        ${INTERPOSITION_FLAG}
        "-lpthread"
)
add_executable(cong_client src/application/checkpoint/cong_client.c src/application/checkpoint/unp.c)
target_link_libraries(cong_client
    PRIVATE
        transport
        ${INTERPOSITION_FLAG}
)