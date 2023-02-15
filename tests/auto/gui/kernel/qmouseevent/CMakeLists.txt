# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qmouseevent Test:
#####################################################################

qt_internal_add_test(tst_qmouseevent
    SOURCES
        tst_qmouseevent.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)
