
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

        # We can't just append to the COMPILE_FLAGS property. That creats a ';' separated list
        # which breaks the compile commmand line.
        # Ensure non-duplication here manually instead.
        get_property(_taget_type TARGET ${_target} PROPERTY TYPE)
        if ("${_taget_type}" STREQUAL "EXECUTABLE")
          get_target_property(_flags ${_target} COMPILE_FLAGS)
          if (_flags)
            list(APPEND _flags ${Qt5${_package}_EXECUTABLE_COMPILE_FLAGS})
            list(REMOVE_DUPLICATES _flags)
          else()
            set(_flags ${Qt5${_package}_EXECUTABLE_COMPILE_FLAGS})
          endif()
          if (_flags)
            set_target_properties(${_target} PROPERTIES COMPILE_FLAGS ${_flags})
          endif()
        endif()
    else()
        message(FATAL_ERROR "NOT FOUND: Qt5${_package}")
    endif()
endmacro()

add_executable(two two.cpp)
add_executable(three three.cpp)

qt5_use_package(two Test)
qt5_use_package(three Widgets)
qt5_use_package(three Test)
