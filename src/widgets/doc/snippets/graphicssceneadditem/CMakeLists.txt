# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

add_library(widgets_graphicssceneadditem_snippets OBJECT
    graphicssceneadditemsnippet.cpp
)

target_link_libraries(widgets_graphicssceneadditem_snippets PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
)
