# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qlatin1stringview Test:
#####################################################################

qt_internal_add_test(tst_qlatin1stringview
    SOURCES
        tst_qlatin1stringview.cpp
    DEFINES
        QT_NO_CAST_TO_ASCII
)

## Scopes:
#####################################################################
