# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qmdisubwindow Test:
#####################################################################

qt_internal_add_test(tst_qmdisubwindow
    SOURCES
        tst_qmdisubwindow.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::Gui
        Qt::Widgets
        Qt::WidgetsPrivate
)
