# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qwindow Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qwindow LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qwindow
    SOURCES
        tst_qwindow.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
)

if(APPLE OR WIN32 OR QT_FEATURE_xcb)
    qt_internal_add_test(tst_foreignwindow
        LOWDPI
        SOURCES
            tst_foreignwindow.cpp
        LIBRARIES
            Qt::CorePrivate
            Qt::Gui
            Qt::GuiPrivate
    )

    if(APPLE)
        enable_language(OBJCXX)
        set_source_files_properties(tst_foreignwindow.cpp PROPERTIES LANGUAGE OBJCXX)
        set_property(TARGET tst_foreignwindow PROPERTY PROPERTY MACOSX_BUNDLE TRUE)
    endif()

    if(QT_FEATURE_xcb)
        target_link_libraries(tst_foreignwindow PRIVATE XCB::XCB)
    endif()
endif()

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qwindow CONDITION QT_FEATURE_dynamicgl AND WIN32
    LIBRARIES
        user32
)
