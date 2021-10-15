cmake_minimum_required(VERSION 3.14)

project(miniTCP)


set(CMAKE_POSITION_INDEPENDENT_CODE on)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_EXTENSION off)

# Include libpcap
include("${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libpcap/FindPCAP.cmake")
# Add subdirectory src.
add_subdirectory(src)
# Add subdirectory 3rdparty
include(FetchContent)
add_subdirectory(3rdparty)

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
            ethernet
    )
    enable_testing()
    add_test(
        NAME test_gtest
        COMMAND $<TARGET_FILE:test_gtest>
    )

    add_executable(test_gmock "")
    target_sources(test_gmock
        PRIVATE
        test_gmock.cc
    )
    target_link_libraries(test_gmock
        PRIVATE
            gmock_main
            utilities
            ethernet
    )
    enable_testing()
    add_test(
        NAME test_gmock
        COMMAND $<TARGET_FILE:test_gmock>
    )

    add_executable(test_common "")
    target_sources(test_common
        PRIVATE
            src/common/logging_test.cc
    )
    target_link_libraries(test_common
        PRIVATE
            gtest_main
            utilities
            ethernet
    )
    enable_testing()
    add_test(
        NAME test_common
        COMMAND $<TARGET_FILE:test_common>
    )

    add_executable(test_ethernet "")
    target_sources(test_ethernet
        PRIVATE
            src/ethernet/mac_addr_test.cc
    )
    target_link_libraries(test_ethernet
        PRIVATE
            gtest_main
            utilities
            ethernet
    )
    enable_testing()
    add_test(
        NAME test_ethernet
        COMMAND $<TARGET_FILE:test_ethernet>
    )
    
endif()

add_executable(main main.cc)
target_link_libraries(main
    PRIVATE
        utilities
        ethernet
        ${PCAP_LIBRARY}
)