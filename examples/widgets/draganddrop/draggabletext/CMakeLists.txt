# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(draggabletext LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

qt_add_executable(draggabletext
    dragwidget.cpp dragwidget.h
    main.cpp
)

set_target_properties(draggabletext PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(draggabletext PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

if(EMSCRIPTEN)
    target_link_options(draggabletext PUBLIC -sASYNCIFY -Os)
endif()

# Resources:
set(draggabletext_resource_files
    "words.txt"
)

qt_add_resources(draggabletext "draggabletext"
    PREFIX
        "/dictionary"
    FILES
        ${draggabletext_resource_files}
)

install(TARGETS draggabletext
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET draggabletext
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
