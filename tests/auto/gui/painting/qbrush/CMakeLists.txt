# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qbrush Test:
#####################################################################

qt_internal_add_test(tst_qbrush
    SOURCES
        tst_qbrush.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)
