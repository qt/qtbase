
find_package(Qt5Core REQUIRED)

add_executable(exe2 "${CMAKE_CURRENT_SOURCE_DIR}/../main.cpp")
target_link_libraries(exe2 Qt5::Core)
