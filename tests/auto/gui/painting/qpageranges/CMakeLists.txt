# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qpageranges Test:
#####################################################################

qt_internal_add_test(tst_qpageranges
    SOURCES
        tst_qpageranges.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)
