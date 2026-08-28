# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(qlocale LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## qlocale Binary:
#####################################################################

qt_internal_add_manual_test(qlocale
    GUI
    SOURCES
        calendar.cpp calendar.h
        currency.cpp currency.h
        dateformats.cpp dateformats.h
        info.cpp info.h
        languages.cpp languages.h
        main.cpp
        miscellaneous.cpp miscellaneous.h
        numberformats.cpp numberformats.h
        window.cpp window.h
    NO_PCH_SOURCES
        languages.cpp # undef QT_NO_FOREACH
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
