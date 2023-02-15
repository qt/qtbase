# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qcompleter Test:
#####################################################################

qt_internal_add_test(tst_qcompleter
    SOURCES
        tst_qcompleter.cpp
    LIBRARIES
        Qt::Gui
        Qt::TestPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)
