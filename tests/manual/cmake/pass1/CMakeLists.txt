
cmake_minimum_required(VERSION 2.8)

project(pass1)

# Warning: requires cmake 2.8.7 due for release in December 2011
# for automatic moc with Qt 5
set(CMAKE_AUTOMOC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

macro(qt5_use_package _target _package)
    # TODO: Handle public/private keywords?
    find_package(Qt5${_package} ${ARG1})
    if (Qt5${_package}_FOUND)
        target_link_libraries(${_target} ${Qt5${_package}_LIBRARY})
        include_directories(${Qt5${_package}_INCLUDE_DIRS})
        # set_property(TARGET ${_target} APPEND PROPERTY INCLUDE_DIRECTORIES ${Qt5${_package}_INCLUDE_DIRS})
        set_property(TARGET ${_target} APPEND PROPERTY COMPILE_DEFINITIONS ${Qt5${_package}_COMPILE_DEFINITIONS})
        foreach(_dep ${_Qt5_MODULE_DEPENDENCIES})
            qt5_use_package(${_target} ${_dep} REQUIRED)
        endforeach()
    endif()
endmacro()

add_executable(two two.cpp)
add_executable(three three.cpp)

qt5_use_package(two Test)
qt5_use_package(three Widgets)
qt5_use_package(three Test)
