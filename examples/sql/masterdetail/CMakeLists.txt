# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(masterdetail LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Sql Widgets Xml)

qt_standard_project_setup()

qt_add_executable(masterdetail
    database.h
    dialog.cpp dialog.h
    main.cpp
    mainwindow.cpp mainwindow.h
)

set_target_properties(masterdetail PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(masterdetail PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Sql
    Qt6::Widgets
    Qt6::Xml
)

# Resources:
set(masterdetail_resource_files
    "images/icon.png"
    "images/image.png"
    "albumdetails.xml"
)

qt_add_resources(masterdetail "masterdetail"
    PREFIX
        "/"
    FILES
        ${masterdetail_resource_files}
)

install(TARGETS masterdetail
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET masterdetail
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
