# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qwidgetmetatype Test:
#####################################################################

qt_internal_add_test(tst_qwidgetmetatype
    SOURCES
        tst_qwidgetmetatype.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
