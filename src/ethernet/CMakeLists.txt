project(link)

add_library(ethernet
    STATIC
        device.cc
        frame_factory.cc
        mac_addr.cc
)

target_include_directories(ethernet
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)