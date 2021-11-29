cmake_minimum_required(VERSION 3.16)

project(network_cppsnippets)

add_executable(mytarget
    network/tcpwait.cpp
)

# ![0]
find_package(Qt6 REQUIRED COMPONENTS Network)
target_link_libraries(mytarget PRIVATE Qt6::Network)
# ![0]
