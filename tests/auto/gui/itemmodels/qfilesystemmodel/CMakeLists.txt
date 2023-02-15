# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qfilesystemmodel Test:
#####################################################################

qt_internal_add_test(tst_qfilesystemmodel
    SOURCES
        tst_qfilesystemmodel.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::Widgets
        Qt::WidgetsPrivate
        Qt::TestPrivate
)
