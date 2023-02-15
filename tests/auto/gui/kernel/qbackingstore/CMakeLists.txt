# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qbackingstore Test:
#####################################################################

qt_internal_add_test(tst_qbackingstore
    SOURCES
        tst_qbackingstore.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)
