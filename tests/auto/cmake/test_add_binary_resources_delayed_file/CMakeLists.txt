
cmake_minimum_required(VERSION 2.8)

project(test_add_binary_resources_delayed_file)

find_package(Qt5Core REQUIRED)

include_directories(${Qt5Core_INCLUDE_DIRS})

add_definitions(${Qt5Core_DEFINITIONS})

qt5_add_binary_resources(rcc_file "${CMAKE_CURRENT_BINARY_DIR}/test_add_binary_resources_delayed_file.qrc" "${CMAKE_CURRENT_SOURCE_DIR}/existing.qrc")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/test_add_binary_resources_delayed_file.qrc" "<!DOCTYPE RCC><RCC version=\"1.0\">
<qresource prefix=\"/\">
    <file alias=\"resource_file.txt\">${CMAKE_CURRENT_SOURCE_DIR}/resource_file.txt</file>
</qresource>
</RCC>
")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")

add_executable(test_add_binary_resources_delayed_file main.cpp)
target_link_libraries(test_add_binary_resources_delayed_file ${Qt5Core_LIBRARIES})
