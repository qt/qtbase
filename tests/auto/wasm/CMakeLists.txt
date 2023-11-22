# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_wasm Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_localfileapi LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_localfileapi
    SOURCES
        tst_localfileapi.cpp
    DEFINES
        QT_NO_FOREACH
    LIBRARIES
        Qt::GuiPrivate
        Qt::Core
        Qt::Gui
        Qt::Widgets
)

qt_internal_add_test(tst_qwasmwindowstack
    SOURCES
        tst_qwasmwindowstack.cpp
    DEFINES
        QT_NO_FOREACH
    LIBRARIES
        Qt::GuiPrivate
        Qt::Core
        Qt::Gui
        Qt::Widgets
)

qt_internal_add_test(tst_qwasmwindowtreenode
    SOURCES
        tst_qwasmwindowtreenode.cpp
    DEFINES
        QT_NO_FOREACH
    LIBRARIES
        Qt::GuiPrivate
        Qt::Core
        Qt::Gui
        Qt::Widgets
)

qt_internal_add_test(tst_qwasmkeytranslator
    SOURCES
        tst_qwasmkeytranslator.cpp
    DEFINES
        QT_NO_FOREACH
    LIBRARIES
        Qt::GuiPrivate
        Qt::Core
)
