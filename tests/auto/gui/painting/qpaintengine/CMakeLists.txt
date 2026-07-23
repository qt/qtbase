# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qpaintengine Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qpaintengine LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Resources:
set_source_files_properties("../../../shared/resources/QtEmojiTestFont-Regular.ttf"
    PROPERTIES QT_RESOURCE_ALIAS "QtEmojiTestFont-Regular.ttf"
)
set(testdata_resource_files
    "../../../shared/resources/QtEmojiTestFont-Regular.ttf"
)

qt_internal_add_test(tst_qpaintengine
    SOURCES
        tst_qpaintengine.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${testdata_resource_files}
    BUILTIN_TESTDATA
)
