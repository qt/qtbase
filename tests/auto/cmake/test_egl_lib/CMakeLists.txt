
cmake_minimum_required(VERSION 2.8)

project(test_egl_lib)

find_package(Qt5Gui REQUIRED)

include_directories(${Qt5Gui_INCLUDE_DIRS} ${Qt5Gui_EGL_INCLUDE_DIRS} )
add_definitions(${Qt5Gui_DEFINITIONS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Gui_EXECUTABLE_COMPILE_FLAGS}")

add_executable(myobject main.cpp )
target_link_libraries(myobject ${Qt5Gui_LIBRARIES} ${Qt5Gui_EGL_LIBRARIES})
