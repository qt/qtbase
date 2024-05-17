# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(stylesheet LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/widgets/stylesheet")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

qt_add_executable(stylesheet
    main.cpp
    mainwindow.cpp mainwindow.h mainwindow.ui
    stylesheeteditor.cpp stylesheeteditor.h stylesheeteditor.ui
)

set_target_properties(stylesheet PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(stylesheet PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Resources:
set(stylesheet_resource_files
    "images/checkbox_checked.png"
    "images/checkbox_checked_hover.png"
    "images/checkbox_checked_pressed.png"
    "images/checkbox_unchecked.png"
    "images/checkbox_unchecked_hover.png"
    "images/checkbox_unchecked_pressed.png"
    "images/down_arrow.png"
    "images/down_arrow_disabled.png"
    "images/frame.png"
    "images/pagefold.png"
    "images/pushbutton.png"
    "images/pushbutton_hover.png"
    "images/pushbutton_pressed.png"
    "images/radiobutton_checked.png"
    "images/radiobutton_checked_hover.png"
    "images/radiobutton_checked_pressed.png"
    "images/radiobutton_unchecked.png"
    "images/radiobutton_unchecked_hover.png"
    "images/radiobutton_unchecked_pressed.png"
    "images/sizegrip.png"
    "images/spindown.png"
    "images/spindown_hover.png"
    "images/spindown_off.png"
    "images/spindown_pressed.png"
    "images/spinup.png"
    "images/spinup_hover.png"
    "images/spinup_off.png"
    "images/spinup_pressed.png"
    "images/up_arrow.png"
    "images/up_arrow_disabled.png"
    "layouts/default.ui"
    "layouts/pagefold.ui"
    "qss/coffee.qss"
    "qss/default.qss"
    "qss/pagefold.qss"
)

qt_add_resources(stylesheet "stylesheet"
    PREFIX
        "/"
    FILES
        ${stylesheet_resource_files}
)

install(TARGETS stylesheet
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
