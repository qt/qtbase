# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qmacstyle Test:
#####################################################################

qt_internal_add_test(tst_qmacstyle
    SOURCES
        tst_qmacstyle.cpp
    LIBRARIES
        Qt::Gui
        Qt::WidgetsPrivate
)
