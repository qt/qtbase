# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qfactoryloader LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qfactoryloader
    SOURCES
        plugin1/plugininterface1.h
        plugin2/plugininterface2.h
        tst_qfactoryloader.cpp
    LIBRARIES
        Qt::CorePrivate
)

if(ANDROID)
    # QT_ANDROID_EXTRA_PLUGINS requires a list of directories, not files!
    set_target_properties(tst_qfactoryloader PROPERTIES
        QT_ANDROID_EXTRA_PLUGINS "${CMAKE_CURRENT_BINARY_DIR}/bin"
    )
endif()

if(OHOS)
    set_target_properties(tst_qfactoryloader PROPERTIES
        QT_HARMONYOS_EXTRA_PLUGINS
            "tst_qfactoryloader_plugin1;tst_qfactoryloader_plugin2"
    )
endif()

add_subdirectory(plugin1)
add_subdirectory(plugin2)
add_subdirectory(staticplugin)

# we have Q_IMPORT_PLUGIN for these
qt_internal_extend_target(tst_qfactoryloader
    LIBRARIES
        tst_qfactoryloader_staticplugin
)
qt_internal_extend_target(tst_qfactoryloader CONDITION NOT QT_FEATURE_library
    LIBRARIES
        tst_qfactoryloader_plugin1
        tst_qfactoryloader_plugin2
)
