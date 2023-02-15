# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qgraphicsobject Test:
#####################################################################

qt_internal_add_test(tst_qgraphicsobject
    SOURCES
        tst_qgraphicsobject.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::Widgets
)
