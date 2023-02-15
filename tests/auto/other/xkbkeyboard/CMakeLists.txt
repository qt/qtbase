# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_xkbkeyboard Test:
#####################################################################

qt_internal_add_test(tst_xkbkeyboard
    SOURCES
        tst_xkbkeyboard.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
)
