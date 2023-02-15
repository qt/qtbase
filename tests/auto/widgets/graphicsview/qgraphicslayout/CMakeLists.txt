# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qgraphicslayout Test:
#####################################################################

qt_internal_add_test(tst_qgraphicslayout
    SOURCES
        tst_qgraphicslayout.cpp
    DEFINES
        QT_USE_USING_NAMESPACE
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
