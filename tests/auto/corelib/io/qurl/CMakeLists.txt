# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qurl.pro.

#####################################################################
## tst_qurl Test:
#####################################################################

qt_internal_add_test(tst_qurl
    SOURCES
        tst_qurl.cpp
    LIBRARIES
        Qt::Concurrent
        Qt::TestPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qurl CONDITION APPLE
    SOURCES
        tst_qurl_mac.mm
)
