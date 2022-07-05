# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

set(tst_qmetaobject_SOURCES
    tst_qmetaobject.cpp
    forwarddeclared.h
    forwarddeclared.cpp
)

# Generated from qmetaobject.pro.

#####################################################################
## tst_qmetaobject Test:
#####################################################################

qt_internal_add_test(tst_qmetaobject
    SOURCES
        ${tst_qmetaobject_SOURCES}
    LIBRARIES
        Qt::CorePrivate
)

