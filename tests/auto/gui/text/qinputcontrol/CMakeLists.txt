# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qinputcontrol Test:
#####################################################################

qt_internal_add_test(tst_qinputcontrol
    SOURCES
        tst_qinputcontrol.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)
