# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_manual_test(embeddedwindows
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
    GUI
)

qt_internal_extend_target(embeddedwindows CONDITION WIN32
    SOURCES
        app.manifest
)

include(../../shared/nativewindow.cmake)
target_link_libraries(embeddedwindows PRIVATE nativewindow)
