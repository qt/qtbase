# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#####################################################################
## tst_qsharedpointer Test:
#####################################################################

qt_internal_add_test(tst_qsharedpointer
    SOURCES
        forwarddeclared.cpp
        nontracked.cpp
        wrapper.cpp
        tst_qsharedpointer.cpp
    LIBRARIES
        Qt::CorePrivate
)
