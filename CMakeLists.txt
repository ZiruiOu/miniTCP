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