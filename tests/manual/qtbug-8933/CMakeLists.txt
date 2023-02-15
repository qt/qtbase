# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtbug-8933 Binary:
#####################################################################

qt_internal_add_manual_test(qtbug-8933
    GUI
    SOURCES
        main.cpp
        widget.cpp widget.h widget.ui
    LIBRARIES
        Qt::Gui
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)
