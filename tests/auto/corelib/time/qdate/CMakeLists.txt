# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdate Test:
#####################################################################

qt_internal_add_test(tst_qdate
    SOURCES
        tst_qdate.cpp
    DEFINES
        QT_NO_FOREACH
        QT_NO_KEYWORDS
    LIBRARIES
        Qt::CorePrivate
)
