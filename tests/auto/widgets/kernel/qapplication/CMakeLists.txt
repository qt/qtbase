# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qapplication LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

add_subdirectory(desktopsettingsaware)
add_subdirectory(modal)
add_subdirectory(test)

qt_internal_add_executable(apphelper_widgets
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        ../../../corelib/kernel/qcoreapplication/apphelper.cpp
        ../../../corelib/kernel/qcoreapplication/maybeshow.h
    LIBRARIES
        Qt::CorePrivate
        Qt::Widgets
)
set_target_properties(apphelper_widgets PROPERTIES OUTPUT_NAME apphelper)
set_target_properties(tst_qapplication PROPERTIES
    QT_ANDROID_EXTRA_LIBS ${CMAKE_CURRENT_BINARY_DIR}/libmodal_helper_${ANDROID_ABI}.so
)

if(QT_FEATURE_library)
    qt_internal_add_cmake_library(apphelper_widgets_plugin
        MODULE
        SOURCES
            ../../../corelib/kernel/qcoreapplication/apphelper_plugin.cpp
        NO_PCH_SOURCES
            ../../../corelib/kernel/qcoreapplication/apphelper_plugin.cpp
        LIBRARIES
            Qt::Widgets
    )
    qt_autogen_tools_initial_setup(apphelper_widgets_plugin)
    add_dependencies(apphelper_widgets apphelper_widgets_plugin)
endif()

add_dependencies(tst_qapplication
    desktopsettingsaware_helper
    modal_helper
    apphelper_widgets
)

qt_internal_add_test(tst_static_qapplication
    SOURCES
        ../../../corelib/kernel/qcoreapplication/tst_static_qcoreapplication.cpp
        ../../../corelib/kernel/qcoreapplication/maybeshow.h
    LIBRARIES
        Qt::CorePrivate
        Qt::Widgets
)
