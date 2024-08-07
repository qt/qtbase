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

    # Test case: ui_mainwindow.h is included as
    # "../../../../src/ui_files/ui_mainwindow.h".
    # Test case: mainwindow.ui file is touched after the first build.
    # Test case: mainwindow.ui is also passed to qt_add_resources.
    # Expect 1: Successful build without the double build issue.
    # Expect 2: No build folder leakage and generation of the "_/_/_/_/"
    # folder in ${hash_folder}/include.
    string(CONCAT test_ui_file "${CMAKE_CURRENT_SOURCE_DIR}/../test_qt_add_ui_"
        "common/uic_test/mainwindow.ui")
    generate_hash_folder("example" "${test_ui_file}" hash_folder)
    string(CONCAT test_build_dir "${CMAKE_CURRENT_BINARY_DIR}/UicBuildLeak_"
        "subFolderGen${config_path}-build")
    string(CONCAT test_file_to_touch "${CMAKE_CURRENT_SOURCE_DIR}/../"
        "test_qt_add_ui_common/uic_test/mainwindow.ui")
    string(CONCAT test_file_to_check "${test_build_dir}/.qt/${hash_folder}/"
        "${config_arg}/src/ui_files/ui_mainwindow.h")
    incremental_build_test(
        TEST_NAME uic_with_rcc
        SOURCE_DIR
            "${CMAKE_CURRENT_SOURCE_DIR}/../test_qt_add_ui_common/uic_test"
        BUILD_DIR "${test_build_dir}"
        CONFIG "${config_arg}"
        GENERATOR "${generator}"
        ADDITIONAL_ARGS "-DMAINWINDOW_UI_PATH=../../../../src/ui_files/"
            "-DTEST_WITH_QRC=ON"
        FILE_TO_TOUCH "${test_file_to_touch}"
        FILE_TO_CHECK "${test_file_to_check}"
        FOLDER_TO_CHECK
            "${test_build_dir}/.qt/${hash_folder}/${config_arg}/_/_/_/_"
    )
    endforeach()
endforeach()
