# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(test)

include(../test_qt_add_ui_common/RunCMake.cmake)
include(../test_qt_add_ui_common/functions.cmake)

get_generators(generators)

foreach(generator IN ITEMS ${generators})
    message(STATUS "Running tests for generator: ${generator}")
    is_multi_config(${generator} multi_config_out)
    if(multi_config_out)
        set(configs "Debug" "Release")
    else()
        set(configs "single_config")
    endif()

    foreach(config IN ITEMS ${configs})
        if("${config}" STREQUAL "single_config")
            set(config_path "")
            set(config_arg "")
        else()
            set(config_path "_${config}")
            set(config_arg "${config}")
        endif()

        # Test case: Check whether api sets warning against AUTOUIC activation.
        # Expect 1: Printing of the error message and failure of the build.
        string(CONCAT test_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/../"
            "test_qt_add_ui_common/uic_test")
        string(CONCAT test_build_dir "${CMAKE_CURRENT_BINARY_DIR}/"
            "uic_warning_test${config_path}-build")
        run_cmake_configure(SOURCE_DIR "${test_source_dir}"
            BUILD_DIR "${test_build_dir}"
            GENERATOR "${generator}"
            CLEAN_FIRST
            ADDITIONAL_ARGS "-DCMAKE_AUTOUIC=ON"
            OUTPUT_VARIABLE cmake_output
            ERROR_VARIABLE cmake_error
            RESULT_VARIABLE cmake_result)
        if(NOT cmake_result EQUAL 0)
            message(FATAL_ERROR "cmake_output: ${cmake_output}\ncmake_error: "
                "${cmake_error}\nFAIL: \"uic_warning_test\" test in "
                "${CMAKE_CURRENT_BINARY_DIR}/uic_warning_test${config_path}-"
                "build failed to configure")
        else()
            message(STATUS "PASS: \"uic_warning_test\" test in "
                "${CMAKE_CURRENT_BINARY_DIR}/uic_warning_test${config_path}-"
                "build was configured successfully")
        endif()

        string(CONCAT test_build_dir "${CMAKE_CURRENT_BINARY_DIR}/"
            "uic_warning_test${config_path}-build")
        run_cmake_build(
            BUILD_DIR "${test_build_dir}"
            VERBOSE ON
            CONFIG "${config_arg}"
            OUTPUT_VARIABLE cmake_build_output
            RESULT_VARIABLE cmake_build_result)
        if(NOT cmake_build_result EQUAL 0)
            message(STATUS "PASS: \"uic_warning_test\" test in "
                "${CMAKE_CURRENT_BINARY_DIR}/uic_warning_test${config_path}"
                "-build failed to build")
        else()
            message(FATAL_ERROR "FAIL: \"uic_warning_test\" test in "
                "${CMAKE_CURRENT_BINARY_DIR}/uic_warning_test${config_path}-"
                "build was built successfully")
        endif()
        expect_string_contains("${cmake_build_output}" "has \"AUTOUIC\" enabled"
            SUCCESS_MESSAGE "\"uic_warning_test\" test in \
${CMAKE_CURRENT_BINARY_DIR}/uic_warning_test${config_path}-build \
has \"has AUTOUIC enabled\""
            FAILURE_MESSAGE "\"uic_warning_test\" test in \
${CMAKE_CURRENT_BINARY_DIR}/uic_warning_test${config_path}-build \
does not have \"has AUTOUIC enabled\"")


        if("${generator}" MATCHES "Ninja")
            # Test case: If INCLUDE_PREFIX is changed without changing the
            # corresponding include path in the source file and Ninja generator
            # is used, this casues the double build issue.
            # Note: Only happens in Ninja generator.
            # Expect 1: Failure of the build in the first build.
            string(CONCAT test_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/../"
                "test_qt_add_ui_common/uic_test")
            string(CONCAT test_build_dir "${CMAKE_CURRENT_BINARY_DIR}/"
                "uic_double_build_test${config_path}-build")
            run_cmake_configure(SOURCE_DIR "${test_source_dir}"
                BUILD_DIR "${test_build_dir}"
                GENERATOR "${generator}"
                CLEAN_FIRST
                ADDITIONAL_ARGS "-DMAINWINDOW_UI_PATH=sub1/sub2/sub3/"
                    "-DDO_NOT_GENERATE_FILE=ON"
                OUTPUT_VARIABLE cmake_output
                ERROR_VARIABLE cmake_error
                RESULT_VARIABLE cmake_result)

            if(NOT cmake_result EQUAL 0)
                message(FATAL_ERROR "cmake_output: ${cmake_output}\ncmake_error"
                    ": ${cmake_error}\nFAIL: \"uic_double_build_test\" test in "
                    "${CMAKE_CURRENT_BINARY_DIR}/uic_double_build_test"
                    "${config_path}-build failed to configure")
            else()
                message(STATUS "PASS: \"uic_double_build_test\" test in "
                    "${CMAKE_CURRENT_BINARY_DIR}/uic_double_build_test"
                    "${config_path}-build was configured successfully")
            endif()

            string(CONCAT test_build_dir "${CMAKE_CURRENT_BINARY_DIR}/"
                "uic_double_build_test${config_path}-build")
            run_cmake_build(
                BUILD_DIR "${test_build_dir}"
                VERBOSE ON
                CONFIG "${config_arg}"
                OUTPUT_VARIABLE cmake_build_output
                RESULT_VARIABLE cmake_build_result)

            if(NOT cmake_build_result EQUAL 0)
                message(FATAL_ERROR "cmake_build_output: ${cmake_build_output}"
                    "FAIL: \"uic_double_build_test\" test in "
                    "${CMAKE_CURRENT_BINARY_DIR}/uic_double_build_test"
                    "${config_path}-build failed to build in the first build")
            else()
                message(STATUS "PASS: \"uic_double_build_test\" test in "
                    "${CMAKE_CURRENT_BINARY_DIR}/uic_double_build_test"
                    "${config_path}-build was built successfully in the first "
                    "build")
            endif()

            string(CONCAT test_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/../"
                "test_qt_add_ui_common/uic_test")
            string(CONCAT test_build_dir "${CMAKE_CURRENT_BINARY_DIR}/"
                "uic_double_build_test${config_path}-build")
            run_cmake_configure(SOURCE_DIR "${test_source_dir}"
                BUILD_DIR "${test_build_dir}"
                GENERATOR "${generator}"
                # We change the INCLUDE_PREFIX here.
                ADDITIONAL_ARGS "-DMAINWINDOW_UI_PATH=sub1/sub2/sub/"
                    "-DDO_NOT_GENERATE_FILE=ON"
                OUTPUT_VARIABLE cmake_output
                ERROR_VARIABLE cmake_error
                RESULT_VARIABLE cmake_result)

            if(NOT cmake_result EQUAL 0)
                message(FATAL_ERROR "cmake_output: ${cmake_output}\ncmake_error"
                    ":${cmake_error}\nFAIL: \"uic_double_build_test\" test in "
                    "${CMAKE_CURRENT_BINARY_DIR}/uic_double_build_test"
                    "${config_path}-build failed to configure in the second "
                    "build")
            else()
                message(STATUS "PASS: \"uic_double_build_test\" test in "
                    "${CMAKE_CURRENT_BINARY_DIR}/uic_double_build_test"
                    "${config_path}-build was configured successfully in the "
                    "second build")
            endif()

            string(CONCAT test_build_dir "${CMAKE_CURRENT_BINARY_DIR}/"
                "uic_double_build_test${config_path}-build")
            run_cmake_build(
                BUILD_DIR "${test_build_dir}"
                VERBOSE ON
                CONFIG "${config_arg}"
                OUTPUT_VARIABLE cmake_build_output
                ERROR_VARIABLE cmake_build_error
                RESULT_VARIABLE cmake_build_result)

            if(NOT cmake_build_result EQUAL 0)
                message(STATUS "PASS: \"uic_double_build_test\" test in"
                    "${CMAKE_CURRENT_BINARY_DIR}/uic_double_build_test"
                    "${config_path}-build failed to build in the first build "
                    "after changing INCLUDE_PREFIX")
            else()
                message(FATAL_ERROR "cmake_build_output: ${cmake_build_output}"
                    "\ncmake_build_error: ${cmake_build_error}\n"
                    "FAIL: \"uic_double_build_test\" test in "
                    "${CMAKE_CURRENT_BINARY_DIR}/uic_double_build_test"
                    "${config_path}-build was built successfully in the first "
                    "build after changing INCLUDE_PREFIX")
            endif()

            set(expected_fail_string "No such file or directory|file not found")
            expect_string_contains(${cmake_build_output}
                "${expected_fail_string}"
                SUCCESS_MESSAGE "\"uic_double_build_test\" test in \
${CMAKE_CURRENT_BINARY_DIR}/uic_double_build_test${config_path}\
-build has \"ui_mainwindow.h: No such file or directory\" in \
the first build after changing INCLUDE_PREFIX"
                FAILURE_MESSAGE "\"uic_double_build_test\" test in \
${CMAKE_CURRENT_BINARY_DIR}/uic_double_build_test${config_path}\
-build does not have \"ui_mainwindow.h: No such file or \
directory\" in the first build after changing INCLUDE_PREFIX")

        endif()
    endforeach()
endforeach()

