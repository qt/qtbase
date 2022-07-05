# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qtemporarydir.pro.

#####################################################################
## tst_qtemporarydir Test:
#####################################################################

qt_internal_add_test(tst_qtemporarydir
    SOURCES
        tst_qtemporarydir.cpp
    LIBRARIES
        Qt::TestPrivate
)

qt_internal_extend_target(tst_qtemporarydir CONDITION WIN32
    LIBRARIES
        shlwapi
)
