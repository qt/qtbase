# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
qt_internal_add_manual_test(hdr
    GUI
    SOURCES
        hdr.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

qt_internal_add_resource(hdr "hdr"
    PREFIX
        "/"
    FILES
        "hdrtexture.vert.qsb"
        "hdrtexture.frag.qsb"
)

set(imgui_base ../shared/imgui)
set(imgui_target hdr)
include(${imgui_base}/imgui.cmakeinc)
