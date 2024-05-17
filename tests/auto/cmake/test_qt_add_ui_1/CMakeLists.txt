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

        # Test case: ui_mainwindow.h is included as
        # "sub1/sub2/sub3/sub4/../../../../src/ui_files/ui_mainwindow.h".
        # Expect 1: Successful build without the double build issue.
        # Expect 2: No build folder leakage and generation of the
        # sub1/sub2/sub3/sub4 folder in ${hash_folder}/include
        string(CONCAT test_ui_file "${CMAKE_CURRENT_SOURCE_DIR}/../"
            "test_qt_add_ui_common/uic_test/mainwindow.ui")
        generate_hash_folder( "example" "${test_ui_file}" hash_folder)
        string(CONCAT test_build_dir
            "${CMAKE_CURRENT_BINARY_DIR}/UicBuildLeak_incPathGen"
            "${config_path}-build")
        string(CONCAT test_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/../"
            "test_qt_add_ui_common/uic_test")
        string(CONCAT test_additional_args "-DMAINWINDOW_UI_PATH=sub1/sub2/"
            "sub3/sub4/../../../../src/ui_files/")
        string(CONCAT test_file_to_check "${test_build_dir}/.qt/${hash_folder}/"
            "${config_arg}/src/ui_files/ui_mainwindow.h")
        string(CONCAT test_folder_to_check "${test_build_dir}/.qt/"
            "${hash_folder}/${config_arg}/sub1/sub2/sub3/sub4")
        incremental_build_test(
            TEST_NAME UicBuildLeak_incPathGen
            SOURCE_DIR "${test_source_dir}"
            BUILD_DIR "${test_build_dir}"
            CONFIG "${config_arg}"
            GENERATOR "${generator}"
            ADDITIONAL_ARGS "${test_additional_args}"
            FILE_TO_TOUCH "${test_ui_file}"
            FILE_TO_CHECK "${test_file_to_check}"
            FOLDER_TO_CHECK "${test_folder_to_check}"
        )
    endforeach()
endforeach()

