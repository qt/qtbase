# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qlibraryinfo LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()


qt_internal_add_test(tst_qlibraryinfo SOURCES tst_qlibraryinfo.cpp
    LIBRARIES
        Qt::CorePrivate

)

qt_add_resources(tst_qlibraryinfo "qtconffiles"
    PREFIX "/"
    FILES
        empty.qt.conf
        partial.qt.conf
        list.qt.conf
        relative-prefix.qt.conf
        merge.qt.conf
        merge-relative-prefix.qt.conf
        merge-absolute-prefix.qt.conf
)

if(MACOS)
    qt_internal_add_executable(bundlehelper
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
        SOURCES
            bundlehelper.cpp
        LIBRARIES
            Qt::Core
    )

    if(QT_FEATURE_framework)
        set(qtcore_library_name "$<TARGET_FILE_NAME:Qt::Core>.framework")
    elseif(QT_FEATURE_shared)
        set(qtcore_library_name "$<TARGET_SONAME_FILE_NAME:Qt::Core>")
    endif()

    if(qtcore_library_name)
        qt_internal_extend_target(tst_qlibraryinfo
            DEFINES
                QTCORE_LIBRARY_NAME="${qtcore_library_name}"
        )
    endif()
endif()
