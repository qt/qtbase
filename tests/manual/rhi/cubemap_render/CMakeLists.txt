# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## cubemap_render Binary:
#####################################################################

qt_internal_add_manual_test(cubemap_render
    GUI
    SOURCES
        cubemap_render.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(cubemap_render_resource_files
    "cubemap_mrt.frag.qsb"
    "cubemap_mrt.vert.qsb"
    "cubemap_oneface.frag.qsb"
    "cubemap_oneface.vert.qsb"
    "cubemap_sample.frag.qsb"
    "cubemap_sample.vert.qsb"
)

qt_internal_add_resource(cubemap_render "cubemap_render"
    PREFIX
        "/"
    FILES
        ${cubemap_render_resource_files}
)
