project(transport)

add_library(transport
    STATIC
        tcp_kernel.cc
        socket_impl.cc
        socket.cc
        tcp_kernel.cc
        callbacks.cc
)

target_include_directories(transport
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(transport
    PRIVATE
        utilities
        ethernet
        network
        ${PCAP_LIB}
)