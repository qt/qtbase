# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsystemtrayicon Test:
#####################################################################

qt_internal_add_test(tst_qsystemtrayicon
    SOURCES
        tst_qsystemtrayicon.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
