# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
qt_internal_add_manual_test(settingseditor
    SOURCES
        locationdialog.cpp locationdialog.h
        main.cpp
        mainwindow.cpp mainwindow.h
        settingstree.cpp settingstree.h
        variantdelegate.cpp variantdelegate.h
    LIBRARIES
        Qt::Widgets
)
