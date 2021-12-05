project(network)

add_library(network
    STATIC
        routing_impl.cc
        routing.cc
        ip.cc
)

target_include_directories(network
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(network
    PRIVATE
        utilities
        ethernet
        ${PCAP_LIBRARY}
)