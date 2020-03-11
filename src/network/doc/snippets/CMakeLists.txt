cmake_minimum_required(VERSION 3.1.0)

project(network_cppsnippets)

add_executable(mytarget
    network/tcpwait.cpp
)

# ![0]
find_package(Qt5 COMPONENTS Network REQUIRED)
target_link_libraries(mytarget Qt5::Network)
# ![0]
