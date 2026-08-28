# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(dialogs LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## dialogs Binary:
#####################################################################

qt_internal_add_manual_test(dialogs
    GUI
    SOURCES
        colordialogpanel.cpp colordialogpanel.h
        filedialogpanel.cpp filedialogpanel.h
        fontdialogpanel.cpp fontdialogpanel.h
        main.cpp
        messageboxpanel.cpp messageboxpanel.h
        utils.cpp utils.h
        wizardpanel.cpp wizardpanel.h
    NO_PCH_SOURCES
        filedialogpanel.cpp # undef QT_NO_FOREACH
        utils.cpp # undef QT_NO_FOREACH
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)

## Scopes:
#####################################################################

qt_internal_extend_target(dialogs CONDITION TARGET Qt::PrintSupport
    LIBRARIES
        Qt::PrintSupport
)

qt_internal_extend_target(dialogs CONDITION NOT (QT_FEATURE_printer EQUAL FALSE)
    SOURCES
        printdialogpanel.cpp printdialogpanel.h printdialogpanel.ui
    NO_PCH_SOURCES
        printdialogpanel.cpp # undef QT_NO_FOREACH
    ENABLE_AUTOGEN_TOOLS
        uic
)
