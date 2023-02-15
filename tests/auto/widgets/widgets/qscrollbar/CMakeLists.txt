# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qscrollbar Test:
#####################################################################

qt_internal_add_test(tst_qscrollbar
    SOURCES
        tst_qscrollbar.cpp
    LIBRARIES
        Qt::Gui
        Qt::TestPrivate
        Qt::Widgets
)
