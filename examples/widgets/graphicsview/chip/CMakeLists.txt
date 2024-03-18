# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(chip LANGUAGES CXX)

find_package(Qt6
    REQUIRED COMPONENTS Core Gui Widgets
    OPTIONAL_COMPONENTS PrintSupport
)

qt_standard_project_setup()

qt_add_executable(chip
    chip.cpp chip.h
    main.cpp
    mainwindow.cpp mainwindow.h
    view.cpp view.h
)

set_target_properties(chip PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(chip PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Resources:
set(images_resource_files
    "fileprint.png"
    "qt4logo.png"
    "rotateleft.png"
    "rotateright.png"
    "zoomin.png"
    "zoomout.png"
)

qt_add_resources(chip "images"
    PREFIX
        "/"
    FILES
        ${images_resource_files}
)

if(TARGET Qt6::PrintSupport)
    target_link_libraries(chip PRIVATE Qt6::PrintSupport)
endif()

install(TARGETS chip
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET chip
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
