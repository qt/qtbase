
cmake_minimum_required(VERSION 2.8)

project(test_openglextensions_module)

find_package(Qt5OpenGLExtensions 5.1.0 REQUIRED)

include_directories(
    ${Qt5OpenGLExtensions_INCLUDE_DIRS}
)

add_definitions(
    ${Qt5OpenGLExtensions_DEFINITIONS}
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5OpenGLExtensions_EXECUTABLE_COMPILE_FLAGS}")

add_executable(mainapp main.cpp)

target_link_libraries(mainapp
    ${Qt5OpenGLExtensions_LIBRARIES}
)
