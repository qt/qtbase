# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(a11y_basic_widgets LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(a11y_basic_widgets
    GUI
    SOURCES
        tabswidget.cpp
        tabswidget.h
        basica11ywidget.h
        basica11ywidget.cpp
        main.cpp
        basic_widgets.html
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Widgets
)

add_custom_target(
    UpdateFile_a11y_basic_widgets ALL
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/basic_widgets.html
            ${CMAKE_CURRENT_BINARY_DIR}/basic_widgets.html
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/basic_widgets.html
)

add_dependencies(UpdateFile_a11y_basic_widgets a11y_basic_widgets)
