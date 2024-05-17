# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(drilldown LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Sql Widgets)

qt_standard_project_setup()

qt_add_executable(drilldown
    ../connection.h
    imageitem.cpp imageitem.h
    informationwindow.cpp informationwindow.h
    main.cpp
    view.cpp view.h
)

set_target_properties(drilldown PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(drilldown PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Sql
    Qt6::Widgets
)

# Resources:
set(drilldown_resource_files
    "images/qt-creator.png"
    "images/qt-logo.png"
    "images/qt-project.png"
    "images/qt-quick.png"
)

qt_add_resources(drilldown "drilldown"
    PREFIX
        "/"
    FILES
        ${drilldown_resource_files}
)

install(TARGETS drilldown
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET drilldown
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
