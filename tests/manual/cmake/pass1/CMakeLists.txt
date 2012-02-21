
cmake_minimum_required(VERSION 2.8)

project(pass1)

set(CMAKE_AUTOMOC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

macro(qt5_use_package _target _package)
    if (NOT Qt5${_package}_FOUND)
        find_package(Qt5${_package} ${ARG1})
    endif()
    if (Qt5${_package}_FOUND)
        # TODO: Handle public/private keywords?
        target_link_libraries(${_target} ${Qt5${_package}_LIBRARIES})
        # ### Requires CMake 2.8.8:
        # set_property(TARGET ${_target} APPEND PROPERTY INCLUDE_DIRECTORIES ${Qt5${_package}_INCLUDE_DIRS})
        include_directories(${Qt5${_package}_INCLUDE_DIRS})
        set_property(TARGET ${_target} APPEND PROPERTY COMPILE_DEFINITIONS ${Qt5${_package}_COMPILE_DEFINITIONS})
        set_property(TARGET ${_target} APPEND PROPERTY COMPILE_FLAGS ${Qt5${_package}_COMPILE_FLAGS})
    else()
        message(FATAL_ERROR "NOT FOUND: Qt5${_package}")
    endif()
endmacro()

add_executable(two two.cpp)
add_executable(three three.cpp)

qt5_use_package(two Test)
qt5_use_package(three Widgets)
qt5_use_package(three Test)
