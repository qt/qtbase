# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# These macros are inspired by ECM:

# a macro for tests that have a simple format where the name matches the
# directory and project

# The following macros will produce tests that generate the build
# system for the test project, build it and then run its tests.
macro(add_cmake_test_generate_build_run_variant name base command)
    string(REPLACE "." "/" src_dir "${base}")
    string(REPLACE "." "/" build_dir "${name}")
    string(REGEX REPLACE "[^.]*\\." "" proj "${name}")
    add_test(NAME "cmake_${name}"
        COMMAND ${CMAKE_CTEST_COMMAND}
            --build-and-test
                "${CMAKE_CURRENT_SOURCE_DIR}/${src_dir}"
                "${CMAKE_CURRENT_BINARY_DIR}/${build_dir}"
            --build-two-config
            --build-generator ${CMAKE_GENERATOR}
            --build-makeprogram ${CMAKE_MAKE_PROGRAM}
            --build-project ${proj}
            ${${name}_EXTRA_OPTIONS}
            --test-command ${command} ${ARGN})
endmacro()

macro(add_cmake_test_generate_build_run name)
    add_cmake_test_generate_build_run_variant("${name}" "${name}" ${ARGN})
endmacro()

# The following macros will produce tests that just run cmake
# to generate the build system for the test project.
macro(add_cmake_test_generate_variant name base)
    string(REPLACE "." "/" src_dir "${base}")
    string(REPLACE "." "/" build_dir "${name}")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${build_dir}")
    add_test(NAME "cmake_${name}"
        COMMAND "${CMAKE_COMMAND}" "-G${CMAKE_GENERATOR}"
            "-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
            ${${name}_EXTRA_OPTIONS}
            "${CMAKE_CURRENT_SOURCE_DIR}/${src_dir}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${build_dir}")
endmacro()

macro(add_cmake_test_generate name)
    add_cmake_test_generate_variant("${name}" "${name}")
endmacro()

add_cmake_test_generate(features)
add_cmake_test_generate(qt_make_output_file)
