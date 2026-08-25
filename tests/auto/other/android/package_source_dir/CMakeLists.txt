# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_android_package_source_dir LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Package name is generated and written to AndroidManifest.xml
qt_internal_add_test(tst_package_source_dir_not_defined
    SOURCES
        tst_android_package_source_dir.cpp
    DEFINES
        EXPECTED_APP_NAME="tst_package_source_dir_not_defined"
)

# Package name from user-provided AndroidManifest.xml
qt_internal_add_test(tst_package_source_dir_custom_android_manifest
    SOURCES
        tst_android_package_source_dir.cpp
    DEFINES
        EXPECTED_APP_NAME="tst_package_source_dir_custom_android_manifest_my"
)

if(QT_ANDROID_GRADLE_MULTI_MODULE)
    set_target_properties(tst_package_source_dir_custom_android_manifest
        PROPERTIES
            QT_ANDROID_PACKAGE_SOURCE_DIR
            "${CMAKE_CURRENT_SOURCE_DIR}/custom_android_manifest_bundle/app"
    )
else()
    set_target_properties(tst_package_source_dir_custom_android_manifest
        PROPERTIES
            QT_ANDROID_PACKAGE_SOURCE_DIR
            "${CMAKE_CURRENT_SOURCE_DIR}/custom_android_manifest"
    )
endif()

if(QT_ANDROID_GRADLE_MULTI_MODULE)
    # Partial gradle template is only supported by QT_ANDROID_GRADLE_MULTI_MODULE
    #
    # Package name from user-provided gradle.build.in
    qt_internal_add_test(tst_package_source_dir_partial_template
        SOURCES
            tst_android_package_source_dir.cpp
        DEFINES
            EXPECTED_APP_NAME="tst_package_source_dir_partial_template_my"
    )
    set_target_properties(tst_package_source_dir_partial_template
        PROPERTIES
            QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/partial_template/app"
    )
endif()
