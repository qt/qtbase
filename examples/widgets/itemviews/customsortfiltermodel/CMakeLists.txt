# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(customsortfiltermodel LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

qt_add_executable(customsortfiltermodel
    filterwidget.cpp filterwidget.h
    main.cpp
    mysortfilterproxymodel.cpp mysortfilterproxymodel.h
    window.cpp window.h
)

set_target_properties(customsortfiltermodel PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(customsortfiltermodel PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Resources:
set(customsortfiltermodel_resource_files
    "images/find.png"
)

qt_add_resources(customsortfiltermodel "customsortfiltermodel"
    PREFIX
        "/"
    FILES
        ${customsortfiltermodel_resource_files}
)

install(TARGETS customsortfiltermodel
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET customsortfiltermodel
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
