project(utilities)

# add library
add_library(utilities
    STATIC
        logging.cc
        timer.cc
        timer_impl.cc
)

target_include_directories(utilities
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)