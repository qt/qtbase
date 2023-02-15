# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## xcb_gl_integration Binary:
#####################################################################

qt_internal_add_manual_test(xcb_gl_integration
    GUI
    SOURCES
        main.cpp
    INCLUDE_DIRECTORIES
        .
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)
