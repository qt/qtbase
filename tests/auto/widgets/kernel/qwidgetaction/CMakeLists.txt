# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qwidgetaction Test:
#####################################################################

qt_internal_add_test(tst_qwidgetaction
    SOURCES
        tst_qwidgetaction.cpp
    LIBRARIES
        Qt::Gui
        Qt::TestPrivate
        Qt::Widgets
)
