# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(rasterwindow LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui)

qt_standard_project_setup()

qt_add_executable(rasterwindow
    main.cpp
    rasterwindow.cpp rasterwindow.h
)

set_target_properties(rasterwindow PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(rasterwindow PRIVATE
    Qt6::Core
    Qt6::Gui
)

install(TARGETS rasterwindow
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET rasterwindow
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
