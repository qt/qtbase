
find_package(Qt5Core REQUIRED)

add_executable(exe2 "${CMAKE_CURRENT_SOURCE_DIR}/../main.cpp")
include_directories(${Qt5Core_INCLUDE_DIRS})
