# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## noninstanced Binary:
#####################################################################

qt_internal_add_manual_test(noninstanced
    GUI
    SOURCES
        noninstanced.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(noninstanced_resource_files
    "material.frag.qsb"
    "material.vert.qsb"
)

qt_internal_add_resource(noninstanced "noninstanced"
    PREFIX
        "/"
    FILES
        ${noninstanced_resource_files}
)
