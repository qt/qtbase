# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt_internal_add_test(tst_qx11info
    SOURCES
        tst_qx11info.cpp
    LIBRARIES
        Qt::GuiPrivate
        XCB::XCB
)
