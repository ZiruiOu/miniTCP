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

option(ENABLE_UNIT_TESTS on)

set(ENABLE_UNIT_TESTS on)
message(STATUS "Enabling unit test : ${ENABLE_UNIT_TESTS}")
if (ENABLE_UNIT_TESTS)
    # Fetch gtest from mirror.
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://codechina.csdn.net/mirrors/google/googletest
        GIT_TAG release-1.11.0
    )
    FetchContent_MakeAvailable(googletest)

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

    add_executable(test_common "")
    target_sources(test_common
        PRIVATE
            src/common/logging_test.cc
    )
    target_link_libraries(test_common
        PRIVATE
            gtest_main
            utilities
    )
    enable_testing()
    add_test(
        NAME test_common
        COMMAND $<TARGET_FILE:test_common>
    )
    # Fetch gmock from gitlab
endif()

add_executable(main main.cc)
