# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdatawidgetmapper Test:
#####################################################################

qt_internal_add_test(tst_qdatawidgetmapper
    SOURCES
        tst_qdatawidgetmapper.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
        Qt::WidgetsPrivate
)
