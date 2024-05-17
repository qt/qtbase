# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(textedit LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/richtext/textedit")

find_package(Qt6
    REQUIRED COMPONENTS Core Gui Widgets
    OPTIONAL_COMPONENTS PrintSupport
)

qt_standard_project_setup()

qt_add_executable(textedit
    main.cpp
    textedit.cpp textedit.h
)

set_target_properties(textedit PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(textedit PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

if (TARGET Qt6::PrintSupport)
    target_link_libraries(textedit PRIVATE Qt6::PrintSupport)
endif()

# Resources:
set(textedit_resource_files
    "example.html"
    "images/logo32.png"
    "images/mac/checkbox-checked.png"
    "images/mac/checkbox.png"
    "images/mac/editcopy.png"
    "images/mac/editcut.png"
    "images/mac/editpaste.png"
    "images/mac/editredo.png"
    "images/mac/editundo.png"
    "images/mac/exportpdf.png"
    "images/mac/filenew.png"
    "images/mac/fileopen.png"
    "images/mac/fileprint.png"
    "images/mac/filesave.png"
    "images/mac/format-indent-less.png"
    "images/mac/format-indent-more.png"
    "images/mac/textbold.png"
    "images/mac/textcenter.png"
    "images/mac/textitalic.png"
    "images/mac/textjustify.png"
    "images/mac/textleft.png"
    "images/mac/textright.png"
    "images/mac/textunder.png"
    "images/mac/textundercolor.png"
    "images/mac/zoomin.png"
    "images/mac/zoomout.png"
    "images/win/checkbox-checked.png"
    "images/win/checkbox.png"
    "images/win/editcopy.png"
    "images/win/editcut.png"
    "images/win/editpaste.png"
    "images/win/editredo.png"
    "images/win/editundo.png"
    "images/win/exportpdf.png"
    "images/win/filenew.png"
    "images/win/fileopen.png"
    "images/win/fileprint.png"
    "images/win/filesave.png"
    "images/win/format-indent-less.png"
    "images/win/format-indent-more.png"
    "images/win/textbold.png"
    "images/win/textcenter.png"
    "images/win/textitalic.png"
    "images/win/textjustify.png"
    "images/win/textleft.png"
    "images/win/textright.png"
    "images/win/textunder.png"
    "images/win/textundercolor.png"
    "images/win/zoomin.png"
    "images/win/zoomout.png"
)

qt_add_resources(textedit "textedit"
    PREFIX
        "/"
    FILES
        ${textedit_resource_files}
)

install(TARGETS textedit
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
