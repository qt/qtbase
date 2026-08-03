# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_androidtestrunner LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_get_host_info_var_prefix(_host_info_prefix)
set(_androidtestrunner_bin "${QT_HOST_PATH}/${${_host_info_prefix}_BINDIR}/androidtestrunner")

if(NOT EXISTS "${_androidtestrunner_bin}")
    message(STATUS
        "androidtestrunner not found on the host at '${_androidtestrunner_bin}'. "
        "Skipping tst_androidtestrunner. Set QT_HOST_PATH to a Qt host install "
        "that includes the androidtestrunner tool to enable it.")
    return()
endif()

add_subdirectory(testapp)

_qt_internal_android_get_target_android_build_dir(_testapp_dir tst_androidtestrunner_testapp)
set(_testapp_apk "${_testapp_dir}/tst_androidtestrunner_testapp.apk")
set(_testapp_aab "${_testapp_dir}/tst_androidtestrunner_testapp.aab")
set(_testapp_package "org.qtproject.example.tst_androidtestrunner_testapp")

get_target_property(_testapp_wrapper tst_androidtestrunner_testapp
    _qt_android_runner_wrapper_path)

set(_args
    "--cmake" "${CMAKE_COMMAND}"
    "--build-dir" "${CMAKE_BINARY_DIR}"
    "--runner-bin" "${_androidtestrunner_bin}"
    "--testapp-dir" "${_testapp_dir}"
    "--testapp-apk" "${_testapp_apk}"
    "--testapp-package" "${_testapp_package}"
)

if(_testapp_wrapper)
    list(APPEND _args "--testapp-wrapper" "${_testapp_wrapper}")
endif()

if(QT_USE_ANDROID_MODERN_BUNDLE AND EXISTS "${Bundletool_EXECUTABLE}")
    list(APPEND _args "--bundletool" "${Bundletool_EXECUTABLE}")
    list(APPEND _args "--testapp-aab" "${_testapp_aab}")
endif()

qt_internal_create_test_script(
    NAME tst_androidtestrunner
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/tst_androidtestrunner.py"
    ARGS ${_args}
    OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/tst_androidtestrunnerWrapper$<CONFIG>.cmake"
)
