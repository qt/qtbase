# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
