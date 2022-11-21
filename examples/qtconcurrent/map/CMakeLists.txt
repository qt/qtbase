# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(mapdemo LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qtconcurrent/map")

find_package(Qt6 REQUIRED COMPONENTS Concurrent Core Gui)

qt_standard_project_setup()

qt_add_executable(mapdemo
    main.cpp
)

target_link_libraries(mapdemo PRIVATE
    Qt6::Concurrent
    Qt6::Core
    Qt6::Gui
)

install(TARGETS mapdemo
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
