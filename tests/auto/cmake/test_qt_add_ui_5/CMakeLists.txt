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


        if("${generator}" MATCHES "Ninja")
            # Test case: Add a new ui file to CMakeLists.txt after the first
            # build.
            # Expect 1: Expect ${target}_autogen/prefix_info.cmake to be
            # generated as expected.
            string(CONCAT test_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/../"
                "test_qt_add_ui_common/uic_test")
            string(CONCAT test_build_dir "${CMAKE_CURRENT_BINARY_DIR}/"
                "uic_prefix_info_cmake_test${config_path}-build")
            run_cmake_configure(SOURCE_DIR "${test_source_dir}"
                BUILD_DIR "${test_build_dir}"
                GENERATOR "${generator}"
                CLEAN_FIRST
                ADDITIONAL_ARGS "-DMAINWINDOW_UI_PATH=sub1/sub2/sub3/"
                OUTPUT_VARIABLE cmake_output
                ERROR_VARIABLE cmake_error
                RESULT_VARIABLE cmake_result)

            if(NOT cmake_result EQUAL 0)
                message(FATAL_ERROR "cmake_output: ${cmake_output}\ncmake_error"
                    ":${cmake_error}\nFAIL: \"uic_prefix_info_cmake_test\" test"
                    " in ${CMAKE_CURRENT_BINARY_DIR}/uic_prefix_info_cmake_test"
                    "${config_path}-build failed to configure")
            else()
                message(STATUS "PASS: \"uic_prefix_info_cmake_test\" test in"
                    "${CMAKE_CURRENT_BINARY_DIR}/uic_prefix_info_cmake_test"
                    "${config_path}-build was configured successfully")
            endif()

            string(CONCAT test_ui_file "${CMAKE_CURRENT_SOURCE_DIR}/../"
                "test_qt_add_ui_common/uic_test/mainwindow.ui")
            generate_hash_folder("example" "${test_ui_file}"
                hash_folder_mainwindow)
            string(CONCAT expected_prefix_info_cmake_content
                "include_guard()\nset(${hash_folder_mainwindow}_prefix \""
                "sub1/sub2/sub3\")")
            # read the content of the prefix_info.cmake file
            string(CONCAT "${CMAKE_CURRENT_BINARY_DIR}/"
                "uic_prefix_info_cmake_test${config_path}-build")
            file(READ
                "${test_build_dir}/example_autogen/prefix_info.cmake"
                prefix_info_cmake_content)

            if("${prefix_info_cmake_content}" STREQUAL
                "${expected_prefix_info_cmake_content}")
                message(STATUS "PASS: \"uic_prefix_info_cmake_test\" test in "
                    "${CMAKE_CURRENT_BINARY_DIR}/uic_prefix_info_cmake_test"
                    "${config_path}-build has the expected content in "
                    "prefix_info.cmake")
            else()
                message(FATAL_ERROR "FAIL: \"uic_prefix_info_cmake_test\" test "
                    "in ${CMAKE_CURRENT_BINARY_DIR}/uic_prefix_info_cmake_test"
                    "${config_path}-build does not have the expected content in"
                    " prefix_info.cmake")
            endif()

            string(CONCAT test_build_dir "${CMAKE_CURRENT_BINARY_DIR}/"
                "uic_prefix_info_cmake_test${config_path}-build")
            run_cmake_build(
                BUILD_DIR "${test_build_dir}"
                VERBOSE ON
                CONFIG "${config_arg}"
                OUTPUT_VARIABLE cmake_build_output
                ERROR_VARIABLE cmake_build_error
                RESULT_VARIABLE cmake_build_result)

            if(NOT cmake_build_result EQUAL 0)
                message(FATAL_ERROR "cmake_build_output: ${cmake_build_output}"
                    "\ncmake_build_error: ${cmake_build_error}\n"
                    "FAIL: \"uic_prefix_info_cmake_test\" test in "
                    "${CMAKE_CURRENT_BINARY_DIR}/uic_prefix_info_cmake_test"
                    "${config_path}-build failed to build in the first build")
            else()
                message(STATUS "PASS: \"uic_prefix_info_cmake_test\" test in "
                    "${CMAKE_CURRENT_BINARY_DIR}/uic_prefix_info_cmake_test"
                    "${config_path}-build was built successfully in the first "
                    "build")
            endif()

            string(CONCAT test_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/../"
                "test_qt_add_ui_common/uic_test")
            string(CONCAT test_build_dir "${CMAKE_CURRENT_BINARY_DIR}/"
                "uic_prefix_info_cmake_test${config_path}-build")
            run_cmake_configure(SOURCE_DIR "${test_source_dir}"
                BUILD_DIR "${test_build_dir}"
                GENERATOR "${generator}"
                ADDITIONAL_ARGS "-DMAINWINDOW_UI_PATH=sub1/sub2/sub3/"
                    "-DNEW_UI_PATH=sub5/sub6/sub7/ " "-DADD_NEW_UI=ON"
                OUTPUT_VARIABLE cmake_output
                ERROR_VARIABLE cmake_error
                RESULT_VARIABLE cmake_result)

            if(NOT cmake_result EQUAL 0)
                message(FATAL_ERROR "cmake_output: ${cmake_output}\ncmake_error"
                    ": ${cmake_error}\nFAIL: \"uic_prefix_info_cmake_test\" "
                    "test in ${CMAKE_CURRENT_BINARY_DIR}/uic_prefix_info_cmake_"
                    "test ${config_path}-build failed to configure in the"
                    "second build")
            else()
                message(STATUS "PASS: \"uic_prefix_info_cmake_test\" test in"
                    " ${CMAKE_CURRENT_BINARY_DIR}/uic_prefix_info_cmake_test"
                    "${config_path}-build was configured successfully in the "
                    "second build")
            endif()

            string(CONCAT test_ui_file "${CMAKE_CURRENT_SOURCE_DIR}/../"
                "test_qt_add_ui_common/uic_test/subdir/mainwindow.ui")
            generate_hash_folder("example" "${test_ui_file}"
                hash_folder_subdir_mainwindow)
            set(expected_prefix_info_cmake_content
                "include_guard()\nset(${hash_folder_subdir_mainwindow}_prefix \
\"sub5/sub6/sub7\")")
            string(CONCAT test_build_dir "${CMAKE_CURRENT_BINARY_DIR}/"
                "uic_prefix_info_cmake_test${config_path}-build")
            file(READ
                "${test_build_dir}/example_autogen/prefix_info.cmake"
                prefix_info_cmake_content)

            if("${prefix_info_cmake_content}" STREQUAL
                "${expected_prefix_info_cmake_content}")
                message(STATUS "PASS: \"uic_prefix_info_cmake_test\" test in "
                    "${CMAKE_CURRENT_BINARY_DIR}/uic_prefix_info_cmake_test"
                    "${config_path}-build has the expected content in "
                    "prefix_info.cmake")
            else()
                message(FATAL_ERROR "FAIL: \"uic_prefix_info_cmake_test\" test "
                    "in ${CMAKE_CURRENT_BINARY_DIR}/uic_prefix_info_cmake_test"
                    "${config_path}-build does not have the expected content "
                    "in prefix_info.cmake")
            endif()
        endif()
    endforeach()
endforeach()

