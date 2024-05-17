# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(run_cmake_configure)
    set(options CLEAN_FIRST)
    set(oneValueArgs SOURCE_DIR BUILD_DIR RESULT_VARIABLE OUTPUT_VARIABLE
        ERROR_VARIABLE GENERATOR)
    set(multiValueArgs ADDITIONAL_ARGS)

    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}"
        ${ARGN})

    if(NOT arg_SOURCE_DIR)
        message(FATAL_ERROR "SOURCE_DIR not specified")
    endif()

    if(NOT arg_BUILD_DIR)
        message(FATAL_ERROR "BUILD_DIR not specified")
    endif()

    is_multi_config(arg_GENERATOR multi_config_out)
    if (NOT ${multi_config_out})
        set(run_arg_config_arg -Darg_TYPE=Debug)
    endif()

    set(test_project_source_dir ${arg_SOURCE_DIR})
    set(test_project_build_dir ${arg_BUILD_DIR})

    # Make sure that file paths are 'real' paths
    get_filename_component(test_project_source_dir "${test_project_source_dir}"
                           REALPATH)
    get_filename_component(test_project_build_dir "${test_project_build_dir}"
                           REALPATH)

    if(arg_CLEAN_FIRST)
        file(REMOVE_RECURSE "${test_project_build_dir}")
    endif()
    file(MAKE_DIRECTORY "${test_project_build_dir}")

    execute_process(COMMAND
        "${CMAKE_COMMAND}"
        -S "${test_project_source_dir}"
        -B "${test_project_build_dir}"
        -G "${arg_GENERATOR}"
        "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"
        ${run_arg_config_arg}
        ${arg_ADDITIONAL_ARGS}
        RESULT_VARIABLE cmake_result
        OUTPUT_VARIABLE cmake_output
        ERROR_VARIABLE cmake_error
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE
        ECHO_OUTPUT_VARIABLE
        ECHO_ERROR_VARIABLE
    )

    # set output variables
    set(${arg_RESULT_VARIABLE} ${cmake_result} PARENT_SCOPE)
    set(${arg_OUTPUT_VARIABLE} ${cmake_output} PARENT_SCOPE)
    set(${arg_ERROR_VARIABLE} ${cmake_error} PARENT_SCOPE)
endfunction()

function(run_cmake_build)
    set(options VERBOSE)
    set(oneValueArgs CONFIG BUILD_DIR RESULT_VARIABLE OUTPUT_VARIABLE
        ERROR_VARIABLE)
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}"
         ${ARGN})

    if(NOT arg_BUILD_DIR)
        message(FATAL_ERROR "BUILD_DIR not specified")
    endif()

    if (arg_VERBOSE OR arg_VERBOSE STREQUAL "")
        set(arg_VERBOSE_ARG --verbose)
    endif()

    if(arg_CONFIG)
        set(arg_BUILD_CONFIG_ARG --config ${arg_CONFIG})
    endif()

    execute_process(COMMAND ${CMAKE_COMMAND}
        --build ${arg_BUILD_DIR}
        ${arg_VERBOSE_ARG}
        ${arg_BUILD_CONFIG_ARG}
        RESULT_VARIABLE cmake_result
        OUTPUT_VARIABLE cmake_output
        ERROR_VARIABLE cmake_error
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE
        ECHO_OUTPUT_VARIABLE
        ECHO_ERROR_VARIABLE)

    set(${arg_RESULT_VARIABLE} ${cmake_result} PARENT_SCOPE)
    set(${arg_OUTPUT_VARIABLE} ${cmake_output} PARENT_SCOPE)
    set(${arg_ERROR_VARIABLE} ${cmake_error} PARENT_SCOPE)
endfunction()

function(is_multi_config generator output)
    if ("${generator}" MATCHES "Visual Studio" OR "${generator}" MATCHES "Xcode"
        OR "${generator}" MATCHES "Ninja Multi-Config")
        set(${output} TRUE PARENT_SCOPE)
    else()
        set(${output} FALSE PARENT_SCOPE)
    endif()
endfunction()

# check if string includes substring
function(_internal_string_contains output string substring)
    if("${string}" MATCHES "${substring}")
        set(${output} TRUE PARENT_SCOPE)
    else()
        set(${output} FALSE PARENT_SCOPE)
    endif()
endfunction()

function(expect_string_contains string substring)
    set(oneValueArgs SUCCESS_MESSAGE FAILURE_MESSAGE)
    cmake_parse_arguments(expect_string_contains "${options}" "${oneValueArgs}"
    "${multiValueArgs}" ${ARGN})
    _internal_string_contains(result "${string}" "${substring}")
    if("${result}" STREQUAL TRUE)
        if (expect_string_contains_SUCCESS_MESSAGE)
            message(STATUS "PASS: ${expect_string_contains_SUCCESS_MESSAGE}")
        else()
            message(STATUS "PASS: \"${string}\" contains \"${substring}\"")
        endif()
    else()
        if (expect_string_contains_FAILURE_MESSAGE)
            message(FATAL_ERROR
                "FAIL: ${expect_string_contains_FAILURE_MESSAGE}")
        else()
            message(FATAL_ERROR "FAIL: \"${string}\" contains \"${substring}\"")
        endif()
    endif()
endfunction()

function(expect_string_not_contains string substring)
    set(oneValueArgs SUCCESS_MESSAGE FAILURE_MESSAGE)
    cmake_parse_arguments(expect_string_not_contains
        "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    _internal_string_contains(result ${string} ${substring})
    if(${result} STREQUAL FALSE)
        if (expect_string_not_contains_SUCCESS_MESSAGE)
            message(STATUS "PASS: ${expect_string_not_contains_SUCCESS_MESSAGE}")
        else()
            message(STATUS "PASS: \"${string}\" not contains \"${substring}\"")
        endif()
    else()
        if (expect_string_not_contains_FAILURE_MESSAGE)
            message(FATAL_ERROR
                "FAIL: ${expect_string_not_contains_FAILURE_MESSAGE}")
        else()
            message(FATAL_ERROR "FAIL: \"${string}\" contains \"${substring}\"")
        endif()
    endif()
endfunction()
