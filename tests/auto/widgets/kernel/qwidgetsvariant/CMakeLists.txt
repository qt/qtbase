# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qwidgetsvariant Test:
#####################################################################

qt_internal_add_test(tst_qwidgetsvariant
    SOURCES
        tst_qwidgetsvariant.cpp
    INCLUDE_DIRECTORIES
        ../../../other/qvariant_common
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
