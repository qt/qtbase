# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(editabletreemodel LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/itemviews/editabletreemodel")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

qt_add_executable(editabletreemodel
    main.cpp
    mainwindow.cpp mainwindow.h mainwindow.ui
    treeitem.cpp treeitem.h
    treemodel.cpp treemodel.h
)

set_target_properties(editabletreemodel PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(editabletreemodel PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Resources:
set(editabletreemodel_resource_files
    "default.txt"
)

qt_add_resources(editabletreemodel "editabletreemodel"
    PREFIX
        "/"
    FILES
        ${editabletreemodel_resource_files}
)

install(TARGETS editabletreemodel
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
