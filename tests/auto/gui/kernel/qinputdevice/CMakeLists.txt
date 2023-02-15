# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qinputdevice Test:
#####################################################################

qt_internal_add_test(tst_qinputdevice
    SOURCES
        tst_qinputdevice.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)
