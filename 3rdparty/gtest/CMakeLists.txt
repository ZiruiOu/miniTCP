cmake_minimum_required(VERSION 3.11)

# Fetch gtest from gitee mirror.
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://gitee.com/mirrors/googletest
    GIT_TAG release-1.11.0
)
FetchContent_MakeAvailable(googletest)