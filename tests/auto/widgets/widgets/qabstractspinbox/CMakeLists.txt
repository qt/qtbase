# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qabstractspinbox Test:
#####################################################################

qt_internal_add_test(tst_qabstractspinbox
    SOURCES
        tst_qabstractspinbox.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::Widgets
)
