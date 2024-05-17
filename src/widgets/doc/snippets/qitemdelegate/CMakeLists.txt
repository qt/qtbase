# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

add_library(widgets_qitemdelegate_snippets OBJECT
    spinbox-delegate.cpp
)

target_link_libraries(widgets_qitemdelegate_snippets PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
)
