# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qstyle Test:
#####################################################################

qt_internal_add_test(tst_qstyle
    SOURCES
        tst_qstyle.cpp
    LIBRARIES
        Qt::Gui
        Qt::TestPrivate
        Qt::Widgets
)
