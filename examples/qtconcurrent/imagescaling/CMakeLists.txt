# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(imagescaling LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Concurrent Core Gui Network Widgets)

qt_standard_project_setup()

qt_add_executable(imagescaling
    downloaddialog.cpp downloaddialog.h downloaddialog.ui
    imagescaling.cpp imagescaling.h
    main.cpp
)

set_target_properties(imagescaling PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(imagescaling PRIVATE
    Qt6::Concurrent
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Widgets
)

install(TARGETS imagescaling
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET imagescaling
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
