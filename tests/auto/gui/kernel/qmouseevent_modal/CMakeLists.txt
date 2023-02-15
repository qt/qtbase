# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qmouseevent_modal Test:
#####################################################################

qt_internal_add_test(tst_qmouseevent_modal
    SOURCES
        tst_qmouseevent_modal.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
