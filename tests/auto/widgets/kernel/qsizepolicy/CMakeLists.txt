# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsizepolicy Test:
#####################################################################

qt_internal_add_test(tst_qsizepolicy
    SOURCES
        tst_qsizepolicy.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
        Qt::WidgetsPrivate
)

## Scopes:
#####################################################################
