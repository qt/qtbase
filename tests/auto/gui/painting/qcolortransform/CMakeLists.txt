# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#####################################################################
## tst_qcolortransform Test:
#####################################################################

qt_internal_add_test(tst_qcolortransform
    SOURCES
        tst_qcolortransform.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)
