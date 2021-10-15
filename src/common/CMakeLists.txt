project(utilities)

add_library(utilities
    STATIC
        logging.cc
)

target_include_directories(utilities
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)