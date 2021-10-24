cmake_minimum_required(VERSION 3.11)

# Fetch gtest from gitee mirror.
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://www.gitee.com/mirrors/googletest.git
    GIT_TAG release-1.11.0
)
FetchContent_MakeAvailable(googletest)