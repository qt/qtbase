# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(htmlinfo LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/xml/htmlinfo")

find_package(Qt6 REQUIRED COMPONENTS Core)

qt_standard_project_setup()

qt_add_executable(htmlinfo
    main.cpp
)

set_target_properties(htmlinfo PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)

target_link_libraries(htmlinfo PRIVATE
    Qt6::Core
)

# Resources:
set(resources_resource_files
    "apache_org.html"
    "simpleexample.html"
    "trolltech_com.html"
    "w3c_org.html"
    "youtube_com.html"
)

qt_add_resources(htmlinfo "resources"
    PREFIX
        "/"
    FILES
        ${resources_resource_files}
)

install(TARGETS htmlinfo
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
