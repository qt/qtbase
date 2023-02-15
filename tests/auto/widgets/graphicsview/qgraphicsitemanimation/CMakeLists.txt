# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qgraphicsitemanimation Test:
#####################################################################

qt_internal_add_test(tst_qgraphicsitemanimation
    SOURCES
        tst_qgraphicsitemanimation.cpp
    DEFINES
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
