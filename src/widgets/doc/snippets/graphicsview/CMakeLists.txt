# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

add_library(widgets_graphicsview_snippets OBJECT
    graphicsview.cpp
)

target_link_libraries(widgets_graphicsview_snippets PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
    Qt::OpenGL
    Qt::OpenGLWidgets
    Qt::PrintSupport
)
