# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qabstractslider Test:
#####################################################################

qt_internal_add_test(tst_qabstractslider
    SOURCES
        tst_qabstractslider.cpp
    LIBRARIES
        Qt::Gui
        Qt::TestPrivate
        Qt::Widgets
)
