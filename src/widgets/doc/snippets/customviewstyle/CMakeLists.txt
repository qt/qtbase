# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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

set_target_properties(widgets_customviewstyle_snippets PROPERTIES UNITY_BUILD OFF)
