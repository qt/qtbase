# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## buffercopy Binary:
#####################################################################

qt_internal_add_manual_test(buffercopy
    GUI
    SOURCES
        buffercopy.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(buffercopy_resource_files
    "buffer.comp.qsb"
    "main.frag.qsb"
    "main.vert.qsb"
)

qt_internal_add_resource(buffercopy "buffercopy"
    PREFIX
        "/"
    FILES
        ${buffercopy_resource_files}
)
