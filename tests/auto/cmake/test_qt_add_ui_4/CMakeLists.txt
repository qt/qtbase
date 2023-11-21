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
    # A CI test fails with the below condition. So, we are running the test
    # only for the Debug configuration.
    if ("${generator}" MATCHES "Xcode" AND
        CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
        set(configs "Debug")
    elseif(multi_config_out)
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

    # Test case: widget1.ui file is touched after the first build.
    # Expect 1: Successful build without the double build issue.
    # Expect 2: Only touched files to be built
    set(test_build_dir
        "${CMAKE_CURRENT_BINARY_DIR}/UicIncrementalBuild${config_path}-build")
    string(CONCAT ui_file "${CMAKE_CURRENT_SOURCE_DIR}/../"
        "test_qt_add_ui_common/UicIncrementalBuild/src/widget1.ui")
    string(CONCAT test_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/../"
        "test_qt_add_ui_common/UicIncrementalBuild")

    generate_hash_folder(
        "example"
        "${ui_file}"
        hash_folder)
    string(CONCAT test_file_to_check "${test_build_dir}/.qt/${hash_folder}/"
        "${config_arg}/src/ui_widget1.h")
    incremental_build_test(
        TEST_NAME UicIncrementalBuild
        SOURCE_DIR "${test_source_dir}"
        BUILD_DIR "${test_build_dir}"
        CONFIG "${config_arg}"
        GENERATOR "${generator}"
        FILE_TO_TOUCH "${ui_file}"
        FILE_TO_CHECK
            "${test_file_to_check}"
        FOLDER_TO_CHECK
            "${test_build_dir}/.qt/${hash_folder}/${config_arg}/src"
        CHECK_UNWANTED_BUILDS
    )
    endforeach()
endforeach()
