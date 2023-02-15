# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qfontcache Test:
#####################################################################

qt_internal_add_test(tst_qfontcache
    SOURCES
        tst_qfontcache.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)
