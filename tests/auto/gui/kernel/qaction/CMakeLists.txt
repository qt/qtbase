# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qaction_kernel Test:
#####################################################################

qt_internal_add_test(tst_qaction_kernel
    SOURCES
        tst_qaction.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)
