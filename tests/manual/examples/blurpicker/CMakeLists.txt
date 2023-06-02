# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(blurpicker LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/effects/blurpicker")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

qt_add_executable(blurpicker
    blureffect.cpp blureffect.h
    blurpicker.cpp blurpicker.h
    main.cpp
)

set_target_properties(blurpicker PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(blurpicker PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Resources:
set(blurpicker_resource_files
    "images/accessories-calculator.png"
    "images/accessories-text-editor.png"
    "images/background.jpg"
    "images/help-browser.png"
    "images/internet-group-chat.png"
    "images/internet-mail.png"
    "images/internet-web-browser.png"
    "images/office-calendar.png"
    "images/system-users.png"
)

qt_add_resources(blurpicker "blurpicker"
    PREFIX
        "/"
    FILES
        ${blurpicker_resource_files}
)

install(TARGETS blurpicker
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
