# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(simplerhiwidget LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core GuiPrivate Widgets)

qt_standard_project_setup()

qt_add_executable(simplerhiwidget
    examplewidget.cpp examplewidget.h
    main.cpp
)

set_target_properties(simplerhiwidget PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

# needs GuiPrivate to be able to include <rhi/qrhi.h>
target_link_libraries(simplerhiwidget PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::GuiPrivate
    Qt6::Widgets
)

qt_add_resources(simplerhiwidget "simplerhiwidget"
    PREFIX
        "/"
    FILES
        "shader_assets/color.vert.qsb"
        "shader_assets/color.frag.qsb"
)

install(TARGETS simplerhiwidget
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET simplerhiwidget
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
