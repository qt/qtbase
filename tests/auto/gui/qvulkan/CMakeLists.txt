# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qvulkan Test:
#####################################################################

qt_internal_add_test(tst_qvulkan
    SOURCES
        tst_qvulkan.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)
