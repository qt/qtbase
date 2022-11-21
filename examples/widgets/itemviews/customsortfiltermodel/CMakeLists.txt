# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(customsortfiltermodel LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/itemviews/customsortfiltermodel")

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
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
