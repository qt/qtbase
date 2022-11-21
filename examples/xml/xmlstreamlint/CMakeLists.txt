# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(xmlstreamlint LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/xml/xmlstreamlint")

find_package(Qt6 REQUIRED COMPONENTS Core Xml)

qt_standard_project_setup()

qt_add_executable(xmlstreamlint
    main.cpp
)

set_target_properties(xmlstreamlint PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)

target_link_libraries(xmlstreamlint PRIVATE
    Qt6::Core
    Qt6::Xml
)

install(TARGETS xmlstreamlint
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
