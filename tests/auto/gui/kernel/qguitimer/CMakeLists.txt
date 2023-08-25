# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qguitimer Test:
#####################################################################

qt_internal_add_test(tst_qguitimer
    SOURCES
        ../../../corelib/kernel/qtimer/tst_qtimer.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::TestPrivate
)
