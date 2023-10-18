# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

qt_internal_add_manual_test(customtype
    GUI
    SOURCES
        main.cpp
        message.cpp message.h
    LIBRARIES
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)
