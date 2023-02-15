# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## windowtransparency Binary:
#####################################################################

qt_internal_add_manual_test(windowtransparency
    SOURCES
        windowtransparency.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
        Qt::OpenGL
)
