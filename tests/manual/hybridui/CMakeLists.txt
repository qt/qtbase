# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_manual_test(hybridui
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
    GUI
)

qt_internal_extend_target(hybridui CONDITION WIN32
    SOURCES
        app.manifest
)

if(APPLE)
    enable_language(OBJCXX)
    set_source_files_properties(main.cpp PROPERTIES LANGUAGE OBJCXX)
endif()

include(../../shared/nativewindow.cmake)
target_link_libraries(hybridui PRIVATE nativewindow)
