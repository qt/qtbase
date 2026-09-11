# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_manual_test(pushconstants
    GUI
    SOURCES
        pushconstants.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

set(pushconstants_resource_files
    "material.vert.qsb"
    "material.frag.qsb"
)

qt_internal_add_resource(pushconstants "pushconstants"
    PREFIX
        "/"
    FILES
        ${pushconstants_resource_files}
)
