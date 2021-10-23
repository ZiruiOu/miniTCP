project(link)

add_library(ethernet
    STATIC
        device.cc
        device_impl.cc
        ethkernel.cc
        framefactory.cc
        packetio.cc
)

target_include_directories(ethernet
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(ethernet
    PRIVATE
        utilities
)