# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
project(echo_server)
cmake_minimum_required(VERSION 3.19)

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS network)

qt_add_executable(echo_server
    main.cpp
)

target_link_libraries(echo_server PUBLIC
    Qt::Core
    Qt::Network
)
