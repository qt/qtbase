# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## drawindirectcount Binary:
#####################################################################

qt_internal_add_manual_test(drawindirectcount
    GUI
    SOURCES
        drawindirectcount.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(drawindirectcount_resource_files
    "quads.vert.qsb"
    "quads.frag.qsb"
    "count_writer.comp.qsb"
)

qt_internal_add_resource(drawindirectcount "drawindirectcount"
    PREFIX
        "/"
    FILES
        ${drawindirectcount_resource_files}
)
