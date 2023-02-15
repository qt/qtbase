# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdoublespinbox Test:
#####################################################################

qt_internal_add_test(tst_qdoublespinbox
    SOURCES
        tst_qdoublespinbox.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
        Qt::WidgetsPrivate
)
