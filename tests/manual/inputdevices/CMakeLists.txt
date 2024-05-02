# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
project(inputdevices)
cmake_minimum_required(VERSION 3.19)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(inputdevices
    main.cpp
    inputdevicemodel.h inputdevicemodel.cpp
)

set_target_properties(inputdevices PROPERTIES
    AUTOMOC TRUE
)

target_link_libraries(inputdevices PUBLIC
    Qt::Widgets
)
