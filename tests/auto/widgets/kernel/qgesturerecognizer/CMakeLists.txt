# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qgesturerecognizer Test:
#####################################################################

qt_internal_add_test(tst_qgesturerecognizer
    SOURCES
        tst_qgesturerecognizer.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
)
