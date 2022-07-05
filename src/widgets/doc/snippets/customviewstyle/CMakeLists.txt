# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

add_library(widgets_customviewstyle_snippets OBJECT
    ../customstyle/customstyle.h
    ../customstyle/customstyle.cpp
    customviewstyle.cpp
)

target_link_libraries(widgets_customviewstyle_snippets PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
)
