# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.5)
project(JulianGregorianCalendar VERSION 0.1 LANGUAGES CXX)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Core)

qt_standard_project_setup()

include_directories(../common/)

qt_add_executable(JulianGregorianCalendar
    ../common/calendarBackendInterface.h
    main.cpp
)

target_link_libraries(JulianGregorianCalendar
    PRIVATE
        Qt::Widgets
        Qt::Core
)
