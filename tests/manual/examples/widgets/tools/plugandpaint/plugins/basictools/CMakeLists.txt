# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

qt_add_plugin(pnp_basictools
    STATIC
    CLASS_NAME BasicToolsPlugin
    basictoolsplugin.cpp basictoolsplugin.h
)

target_include_directories(pnp_basictools PRIVATE
    ../../app
)

target_link_libraries(pnp_basictools PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)
