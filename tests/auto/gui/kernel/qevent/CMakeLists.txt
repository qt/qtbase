# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qevent Test:
#####################################################################

qt_internal_add_test(tst_qevent
    SOURCES
        tst_qevent.cpp
    LIBRARIES
        Qt::Gui
        Qt::CorePrivate
)
