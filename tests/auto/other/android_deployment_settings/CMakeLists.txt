# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_android_deployment_settings LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

function(tst_generate_android_deployment_setting target)
    # Qt build always use new property format that supports generator expressions. In this test we
    # want to test user scenario so disable all Qt specific options to make sure that
    # qt6_android_generate_deployment_settings behave like in user projects.
    set(QT_BUILD_STANDALONE_TESTS FALSE)
    set(QT_BUILDING_QT FALSE)
    set(QT_INTERNAL_IS_STANDALONE_TEST FALSE)

    qt6_android_generate_deployment_settings(${target})
endfunction()

function(tst_add_android_permissions target)
    qt6_add_android_permission(${target} NAME PERMISSION_WITH_ATTRIBUTES
                                         ATTRIBUTES
                                             minSdkVersion 32 maxSdkVersion 34)
    qt6_add_android_permission(${target} NAME PERMISSION_WITHOUT_ATTRIBUTES)
endfunction()

qt6_policy(SET QTP0002 NEW)

set(target tst_android_deployment_settings_new)
qt6_add_executable(${target} MANUAL_FINALIZATION EXCLUDE_FROM_ALL noop.cpp)
set_target_properties(${target} PROPERTIES
    my_package_source_dir "path/to/source/dir"
    my_libs_property "some/path/to/lib2.so;some/path/to/lib3.so"
    my_plugins_property "some/path/to/plugin2.so;some/path/to/plugin3.so"

    QT_ANDROID_SDK_BUILD_TOOLS_REVISION "23.0.2"
    QT_ANDROID_MIN_SDK_VERSION "1"
    QT_ANDROID_TARGET_SDK_VERSION "2"
    QT_ANDROID_COMPILE_SDK_VERSION "35"
    QT_ANDROID_APP_NAME "Android Deployment Settings Test"
    QT_ANDROID_PACKAGE_NAME "org.qtproject.android_deployment_settings_test"
    QT_ANDROID_DEPLOYMENT_DEPENDENCIES "dep1.so;dep2.so;dep3.so"
    QT_ANDROID_DEPLOYMENT_SETTINGS_FILE "attempt_to_rewrite.json"
    QT_ANDROID_EXTRA_LIBS
        "some/path/to/lib1.so;$<TARGET_PROPERTY:${target},my_libs_property>;some/path/to/lib4.so"
    QT_ANDROID_EXTRA_PLUGINS
        "some/path/to/plugin1.so;$<TARGET_PROPERTY:${target},my_plugins_property>"
    QT_ANDROID_PACKAGE_SOURCE_DIR "$<TARGET_PROPERTY:${target},my_package_source_dir>"
    QT_ANDROID_SYSTEM_LIBS_PREFIX "myLibPrefix"

    # Make sure that this property is overwritten by
    # qt6_android_generate_deployment_settings
    QT_ANDROID_DEPLOYMENT_SETTINGS_FILE "custom_deployment_settings.json"
)
tst_add_android_permissions(${target})
tst_generate_android_deployment_setting(${target})

qt6_policy(SET QTP0002 OLD)

set(target tst_android_deployment_settings_old)
qt6_add_executable(${target} MANUAL_FINALIZATION EXCLUDE_FROM_ALL noop.cpp)
set_target_properties(${target} PROPERTIES
    QT_ANDROID_SDK_BUILD_TOOLS_REVISION "23.0.2"
    QT_ANDROID_MIN_SDK_VERSION "1"
    QT_ANDROID_TARGET_SDK_VERSION "2"
    QT_ANDROID_COMPILE_SDK_VERSION "35"
    QT_ANDROID_APP_NAME "Android Deployment Settings Test"
    QT_ANDROID_PACKAGE_NAME "org.qtproject.android_deployment_settings_test"
    QT_ANDROID_DEPLOYMENT_DEPENDENCIES "dep1.so;dep2.so;dep3.so"
    QT_ANDROID_EXTRA_LIBS
        "some/path/to/lib1.so;some/path\\to/lib2.so;some\\path\\to\\lib3.so;some/path/to/lib4.so"
    QT_ANDROID_EXTRA_PLUGINS
        "some\\path\\to\\plugin1.so;some/path\\to/plugin2.so;some/path/to/plugin3.so"
    QT_ANDROID_PACKAGE_SOURCE_DIR "path\\to/source\\dir"
    QT_ANDROID_SYSTEM_LIBS_PREFIX "myLibPrefix"
)
tst_add_android_permissions(${target})
tst_generate_android_deployment_setting(${target})

get_target_property(new_settings
    tst_android_deployment_settings_new QT_ANDROID_DEPLOYMENT_SETTINGS_FILE)
get_target_property(old_settings
    tst_android_deployment_settings_old QT_ANDROID_DEPLOYMENT_SETTINGS_FILE)
set_source_files_properties("${new_settings}" PROPERTIES QT_RESOURCE_ALIAS "new_settings.json")
set_source_files_properties("${old_settings}" PROPERTIES QT_RESOURCE_ALIAS "old_settings.json")
qt_internal_add_test(tst_android_deployment_settings
    SOURCES
        tst_android_deployment_settings.cpp
    DEFINES
        DEFAULT_ABI="${CMAKE_ANDROID_ARCH_ABI}"
)
qt6_add_resources(tst_android_deployment_settings
    "deployment_settings_res"
    PREFIX
        "/"
    FILES
        "${new_settings}"
        "${old_settings}"
)
