# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

add_library(widgets_filedialogurl_snippets OBJECT
    filedialogurls.cpp
)

target_link_libraries(widgets_filedialogurl_snippets PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
)
