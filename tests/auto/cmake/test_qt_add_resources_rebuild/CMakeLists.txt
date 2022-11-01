# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(test_qt_add_resources_rebuild)

set(test_project_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/sample")
set(test_project_build_dir "${CMAKE_CURRENT_BINARY_DIR}/build_sample")

# Make sure that file paths are 'real' paths
get_filename_component(test_project_source_dir "${test_project_source_dir}" REALPATH)
get_filename_component(test_project_build_dir "${test_project_build_dir}" REALPATH)

file(REMOVE_RECURSE "${test_project_build_dir}")
file(MAKE_DIRECTORY "${test_project_build_dir}")

# For access to _qt_internal_get_cmake_test_configure_options
find_package(Qt6 COMPONENTS Core REQUIRED)
include("${_Qt6CTestMacros}")

set(indent "    ")
list(APPEND CMAKE_MESSAGE_INDENT "${indent}")

function(configure_project)
    message(STATUS "Configuring build")
    _qt_internal_get_cmake_test_configure_options(option_list)
    execute_process(COMMAND
            "${CMAKE_COMMAND}"
            "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"
            "-G${CMAKE_GENERATOR}"
            ${option_list}
            -B "${test_project_build_dir}"
            -S "${test_project_source_dir}"
        RESULT_VARIABLE result
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Unable to configure sample project")
    endif()
endfunction()

function(try_build)
    message(STATUS "Building project")
    execute_process(COMMAND
            "${CMAKE_COMMAND}"
            --build "${test_project_build_dir}"
        RESULT_VARIABLE result
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Unable to build test project")
    endif()
endfunction()

function(get_target_path out_var)
    file(STRINGS "${test_project_build_dir}/targets.txt" targets)
    list(GET targets 0 first_target_path)
    message(STATUS "Built target is at '${first_target_path}'")
    set(${out_var} "${first_target_path}" PARENT_SCOPE)
endfunction()

function(get_timestamp file_path out_var)
    message(STATUS "Getting timestamp of built target.")
    file(TIMESTAMP "${file_path}" value "%s")
    set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

function(sleep)
    # Avoids issues with low resolution modification times (like HFS on macOS).
    set(seconds 2)
    message(STATUS "Sleeping for ${seconds} seconds.")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep ${seconds})
endfunction()

function(touch_file)
    set(input "input.ts")
    set(input_path "${test_project_source_dir}/${input}")
    message(STATUS "Touching ${input_path}")
    file(TOUCH "${input_path}")
endfunction()

function(assert_timestamp_is_equal before after)
    set(timestamps "\n${indent}Before TS: ${before}\n${indent} After TS: ${after}")
    if("${after}" EQUAL "${before}")
        message(STATUS "Target was not rebuilt. ${timestamps}")
    else()
        message(FATAL_ERROR "Target WAS rebuilt. ${timestamps}")
    endif()
endfunction()

function(assert_timestamp_is_greater before after)
    set(timestamps "\n${indent}Before TS: ${before}\n${indent} After TS: ${after}")
    if("${after}" GREATER "${before}")
        message(STATUS "Target was correctly rebuilt. ${timestamps}")
    else()
        message(FATAL_ERROR "Target was NOT rebuilt. ${timestamps}")
    endif()
endfunction()

configure_project()
try_build()
get_target_path(target_path)

# Make sure that a second build without changes doesn't rebuild the executable.
get_timestamp("${target_path}" ts_1)
sleep()
try_build()
get_timestamp("${target_path}" ts_2)
assert_timestamp_is_equal("${ts_1}" "${ts_2}")

# Touching the input file should cause rcc to rerun, then the compiler, then the linker,
# and thus the executable timestamp should be updated.
touch_file()
try_build()
get_timestamp("${target_path}" ts_3)
assert_timestamp_is_greater("${ts_2}" "${ts_3}")

# Check that building again doesn't rebuild the executable.
sleep()
try_build()
get_timestamp("${target_path}" ts_4)
assert_timestamp_is_equal("${ts_3}" "${ts_4}")

list(POP_BACK CMAKE_MESSAGE_INDENT)
