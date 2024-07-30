# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

qt_add_library(calendarPlugin SHARED
    ../common/calendarBackendInterface.h
    calendarplugin.h
    calendarplugin.cpp
    calendarbackend.cpp
    calendarbackend.h
)

target_include_directories(calendarPlugin PRIVATE ../common)

target_link_libraries(calendarPlugin
    PRIVATE
        Qt::Widgets
        Qt::Core
        Qt::CorePrivate
)
