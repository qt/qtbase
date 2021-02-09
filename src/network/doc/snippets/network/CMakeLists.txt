add_library(network_cppsnippets OBJECT tcpwait.cpp)

target_link_libraries(network_cppsnippets PRIVATE
    Qt::Network
)
