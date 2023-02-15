# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qregion Test:
#####################################################################

qt_internal_add_test(tst_qregion
    SOURCES
        tst_qregion.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)
