# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(ios_projects LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

function(create_target target)
    qt_add_executable(${target}
        main.cpp
    )
    set_target_properties(${target} PROPERTIES
        MACOSX_BUNDLE TRUE
    )
    target_link_libraries(${target} PUBLIC
        Qt::Widgets
    )
endfunction()

create_target(simple_widget_app)

