# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## drawindirectdepth Binary:
#####################################################################

qt_internal_add_manual_test(drawindirectdepth
    GUI
    SOURCES
        drawindirectdepth.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(drawindirectdepth_resource_files
    "quads.vert.qsb"
    "quads.frag.qsb"
)

qt_internal_add_resource(drawindirectdepth "drawindirectdepth"
    PREFIX
        "/"
    FILES
        ${drawindirectdepth_resource_files}
)
