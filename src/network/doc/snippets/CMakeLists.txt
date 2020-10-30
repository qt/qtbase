cmake_minimum_required(VERSION 3.1.0)

project(network_cppsnippets)

add_executable(mytarget
    network/tcpwait.cpp
)

# ![0]
find_package(Qt6 COMPONENTS Network REQUIRED)
target_link_libraries(mytarget Qt6::Network)
# ![0]
