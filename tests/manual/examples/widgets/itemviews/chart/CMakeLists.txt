# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(chart LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/itemviews/chart")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

qt_add_executable(chart
    main.cpp
    mainwindow.cpp mainwindow.h
    pieview.cpp pieview.h
)

set_target_properties(chart PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(chart PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Resources:
set(chart_resource_files
    "qtdata.cht"
)

qt_add_resources(chart "chart"
    PREFIX
        "/Charts"
    FILES
        ${chart_resource_files}
)

if(UNIX AND NOT APPLE AND NOT HAIKU AND NOT INTEGRITY AND NOT VXWORKS)
    target_link_libraries(chart PRIVATE
        m
    )
endif()

install(TARGETS chart
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
