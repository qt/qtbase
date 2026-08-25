# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.20)

project(test_android_signing)

enable_testing()

find_program(keytool NAMES keytool)

if(NOT keytool)
    message("Skipping test_android_signing: keytool not found.")
    return()
endif()

if("$ENV{QT_ANDROID_KEYSTORE_PATH}" STREQUAL ""
    OR "$ENV{QT_ANDROID_KEYSTORE_STORE_PASS}" STREQUAL ""
    OR "$ENV{QT_ANDROID_KEYSTORE_ALIAS}" STREQUAL ""
    OR "$ENV{QT_ANDROID_KEYSTORE_KEY_PASS}" STREQUAL "")
    message(FATAL_ERROR "Environment variables QT_ANDROID_KEYSTORE_PATH,"
        " QT_ANDROID_KEYSTORE_STORE_PASS, QT_ANDROID_KEYSTORE_ALIAS, and"
        " QT_ANDROID_KEYSTORE_KEY_PASS must be set to run this test.")
endif()

find_package(Qt6 COMPONENTS Core Gui REQUIRED)

if(NOT EXISTS "$ENV{QT_ANDROID_KEYSTORE_PATH}" OR IS_DIRECTORY "$ENV{QT_ANDROID_KEYSTORE_PATH}")
    execute_process(COMMAND ${keytool}
        -genkey -alias "$ENV{QT_ANDROID_KEYSTORE_ALIAS}" -keyalg RSA
        -keystore "$ENV{QT_ANDROID_KEYSTORE_PATH}"
        -noprompt
        -dname "CN=qttest, OU=ID, O=Qt, L=Qt, S=Test, C=DE"
        -storepass "$ENV{QT_ANDROID_KEYSTORE_STORE_PASS}"
        -keypass "$ENV{QT_ANDROID_KEYSTORE_KEY_PASS}"
        RESULT_VARIABLE keytool_result
    )
endif()


qt6_add_executable(test_android_signing main.cpp)

target_link_libraries(test_android_signing PRIVATE
    Qt6::Core
    Qt6::Gui
)

_qt_internal_android_get_target_deployment_dir(android_build test_android_signing)

if(QT_ANDROID_GRADLE_MULTI_MODULE)
    set(output_package_basename "app-release-signed")
else()
    get_filename_component(build_dir_name ${android_build} NAME)
    if(QT_ANDROID_SIGN_AAB)
        set(output_package_basename "${build_dir_name}-release")
    else()
        set(output_package_basename "${build_dir_name}-release-signed")
    endif()
endif()

if(QT_ANDROID_SIGN_AAB)
    set(expected_output
        "${android_build}/build/outputs/bundle/release/${output_package_basename}.aab")
else()
    set(expected_output
        "${android_build}/build/outputs/apk/release/${output_package_basename}.apk")
endif()

add_test(NAME test_android_signing_verify_signature
    COMMAND "${CMAKE_COMMAND}" "-DKEYTOOL_PATH=${keytool}"
    "-DOUTPUT_FILE=${expected_output}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/verify_signature.cmake"
)
