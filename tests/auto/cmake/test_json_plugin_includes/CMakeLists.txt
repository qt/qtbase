
cmake_minimum_required(VERSION 2.8)

project(test_json_plugin_includes)

find_package(Qt5Core REQUIRED)

include_directories(${Qt5Core_INCLUDE_DIRS})

add_definitions(${Qt5Core_DEFINITIONS})

set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/jsontestplugin.json" "{}\n")

qt5_wrap_cpp(moc_files plugin.h)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")

add_library(plugin plugin.cpp ${moc_files})
target_link_libraries(plugin ${Qt5Core_LIBRARIES})
