# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qguieventdispatcher Test:
#####################################################################

qt_internal_add_test(tst_qguieventdispatcher
    SOURCES
        ../../../corelib/kernel/qeventdispatcher/tst_qeventdispatcher.cpp
    LIBRARIES
        Qt::Gui
)
