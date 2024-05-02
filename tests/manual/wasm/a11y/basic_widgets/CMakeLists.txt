# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
qt_internal_add_manual_test(a11y_basic_widgets
    GUI
    SOURCES
        tabswidget.cpp
        tabswidget.h
        basica11ywidget.h
        basica11ywidget.cpp
        main.cpp
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Widgets
)

add_custom_command(
    TARGET a11y_basic_widgets PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/basic_widgets.html
            ${CMAKE_CURRENT_BINARY_DIR}/basic_widgets.html
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/basic_widgets.html
)
