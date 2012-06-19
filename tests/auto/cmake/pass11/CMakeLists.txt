
cmake_minimum_required(VERSION 2.8)

project(pass11)

find_package(Qt5Core REQUIRED)

include_directories(${Qt5Core_INCLUDE_DIRS})

add_definitions(${Qt5Core_DEFINITIONS})

qt5_add_resources(rcc_files "${CMAKE_CURRENT_BINARY_DIR}/pass11.qrc")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/pass11.qrc" "<!DOCTYPE RCC><RCC version=\"1.0\">
<qresource prefix=\"/\">
    <file>${CMAKE_CURRENT_SOURCE_DIR}/resource_file.txt</file>
</qresource>
</RCC>
")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")

add_executable(pass11 main.cpp ${rcc_files})
target_link_libraries(pass11 ${Qt5Core_LIBRARIES})
