project(network)

add_library(network
    STATIC
        arp_impl.cc
        arp.cc
)

target_include_directories(network
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(network
    PRIVATE
        utilities
        ethernet
)