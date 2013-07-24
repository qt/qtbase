
cmake_minimum_required(VERSION 2.8)

project(test_private_includes)

find_package(Qt5Gui REQUIRED Private)

include_directories(
  ${Qt5Gui_INCLUDE_DIRS}
  ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
)

add_definitions(${Qt5Gui_DEFINITIONS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Gui_EXECUTABLE_COMPILE_FLAGS}")

add_executable(testapp main.cpp)
target_link_libraries(testapp ${Qt5Gui_LIBRARIES})
