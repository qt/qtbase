# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## shadowmap Binary:
#####################################################################

qt_internal_add_manual_test(shadowmap
    GUI
    SOURCES
        shadowmap.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(shadowmap_resource_files
    "main.frag.qsb"
    "main.vert.qsb"
    "shadowmap.frag.qsb"
    "shadowmap.vert.qsb"
)

qt_internal_add_resource(shadowmap "shadowmap"
    PREFIX
        "/"
    FILES
        ${shadowmap_resource_files}
)
