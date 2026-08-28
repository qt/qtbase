# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## indirectcommandbuffer Binary:
#####################################################################

qt_internal_add_manual_test(indirectcommandbuffer
    GUI
    SOURCES
        indirectcommandbuffer.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(indirectcommandbuffer_resource_files
    "quads.vert.qsb"
    "quads.frag.qsb"
    "buildcmds.comp.qsb"
)

qt_internal_add_resource(indirectcommandbuffer "indirectcommandbuffer"
    PREFIX
        "/"
    FILES
        ${indirectcommandbuffer_resource_files}
)

set(imgui_base ../shared/imgui)
set(imgui_target indirectcommandbuffer)
include(${imgui_base}/imgui.cmakeinc)
