# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qboxlayout Test:
#####################################################################

qt_internal_add_test(tst_qboxlayout
    SOURCES
        tst_qboxlayout.cpp
    LIBRARIES
        Qt::Gui
        Qt::TestPrivate
        Qt::Widgets
)
