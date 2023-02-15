# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_manual_graphicsview Binary:
#####################################################################

qt_internal_add_manual_test(tst_manual_graphicsview
    GUI
    SOURCES
        gestures.cpp gestures.h
        imageitem.cpp imageitem.h
        main.cpp
        mousepangesturerecognizer.cpp mousepangesturerecognizer.h
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
