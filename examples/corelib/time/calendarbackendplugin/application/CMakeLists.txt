# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

qt_add_executable(JulianGregorianCalendar
    ../common/calendarBackendInterface.h
    main.cpp
)

target_include_directories(JulianGregorianCalendar PRIVATE ../common)

target_link_libraries(JulianGregorianCalendar
    PRIVATE
        Qt::Widgets
        Qt::Core
)

