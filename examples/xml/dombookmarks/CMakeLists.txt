# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(dombookmarks LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Xml)

qt_standard_project_setup()

qt_add_executable(dombookmarks
    main.cpp
    mainwindow.cpp mainwindow.h
    xbeltree.cpp xbeltree.h
)

set_target_properties(dombookmarks PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(dombookmarks PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Xml
)

install(TARGETS dombookmarks
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET dombookmarks
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
