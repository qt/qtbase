
cmake_minimum_required(VERSION 2.8)

project(test_concurrent_module)

find_package(Qt5Concurrent 5.0.0 REQUIRED)

include_directories(
    ${Qt5Concurrent_INCLUDE_DIRS}
)

add_definitions(
    ${Qt5Concurrent_DEFINITIONS}
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")

add_executable(mainapp main.cpp)

target_link_libraries(mainapp
    ${Qt5Concurrent_LIBRARIES}
)
