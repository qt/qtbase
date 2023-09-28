# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

project(tablet_device_info)
cmake_minimum_required(VERSION 3.19)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(tablet_device_info
    main.cpp
    tabletwidget.h tabletwidget.cpp
)

set_target_properties(tablet_device_info PROPERTIES
    AUTOMOC TRUE
)

target_link_libraries(tablet_device_info PUBLIC
    Qt::Widgets
)
