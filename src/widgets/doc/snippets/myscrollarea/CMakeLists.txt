# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_library(widgets_myscrollarea_snippets OBJECT
    myscrollarea.cpp
)

target_link_libraries(widgets_myscrollarea_snippets PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
)
