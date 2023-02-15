# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qhighdpi Test:
#####################################################################

qt_internal_add_test(tst_qhighdpi
    SOURCES
        tst_qhighdpi.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)
