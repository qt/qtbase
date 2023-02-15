# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## instancing Binary:
#####################################################################

qt_internal_add_manual_test(tst_manual_instancing
    GUI
    SOURCES
        instancing.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(instancing_resource_files
    "inst.frag.qsb"
    "inst.vert.qsb"
)

qt_internal_add_resource(tst_manual_instancing "instancing"
    PREFIX
        "/"
    FILES
        ${instancing_resource_files}
)
