# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_add_plugin(pnp_basictools
    STATIC
    CLASS_NAME BasicToolsPlugin
    basictoolsplugin.cpp basictoolsplugin.h
)

target_include_directories(pnp_basictools PUBLIC
    ../../app
)

target_link_libraries(pnp_basictools PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
)
