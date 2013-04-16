
cmake_minimum_required(VERSION 2.8)

project(test_opengl_lib)

find_package(Qt5Gui REQUIRED)

include_directories(${Qt5Gui_INCLUDE_DIRS} ${Qt5Gui_OPENGL_INCLUDE_DIRS} )
add_definitions(${Qt5Gui_DEFINITIONS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Gui_EXECUTABLE_COMPILE_FLAGS}")

if (${Qt5Gui_OPENGL_IMPLEMENTATION} STREQUAL GLES)
    add_definitions(-DGL_IMPLEMENTATION_GLES1)
elseif (${Qt5Gui_OPENGL_IMPLEMENTATION} STREQUAL GLESv2)
    add_definitions(-DGL_IMPLEMENTATION_GLES2)
elseif (${Qt5Gui_OPENGL_IMPLEMENTATION} STREQUAL GL)
    add_definitions(-DGL_IMPLEMENTATION_GL)
else()
    message(SEND_ERROR "Qt5Gui_OPENGL_IMPLEMENTATION does not contain valid data.")
endif()

add_executable(myobject main.cpp )
target_link_libraries(myobject ${Qt5Gui_LIBRARIES} ${Qt5Gui_OPENGL_LIBRARIES})
