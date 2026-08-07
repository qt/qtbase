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

# On WebAssembly the plugin is a side module. One way to make it available to
# the app is to preload it into the in-memory filesystem at startup.
if(EMSCRIPTEN)
    qt_add_wasm_preload(JulianGregorianCalendar
        DESTINATION /plugin
        TARGETS calendarPlugin
    )
endif()

