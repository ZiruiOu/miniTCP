project(link)

add_library(ethernet
    STATIC
        device.cc
        framefactory.cc
        macaddr.cc
)

target_include_directories(ethernet
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(ethernet
    PRIVATE
        utilities
)