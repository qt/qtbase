# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qwindow Test:
#####################################################################

qt_internal_add_test(tst_qwindow
    SOURCES
        tst_qwindow.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qwindow CONDITION QT_FEATURE_dynamicgl AND WIN32
    LIBRARIES
        user32
)
