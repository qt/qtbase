# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtime Test:
#####################################################################

qt_internal_add_test(tst_qtime
    SOURCES
        tst_qtime.cpp
    DEFINES
        QT_NO_FOREACH
        QT_NO_KEYWORDS
    LIBRARIES
        Qt::CorePrivate
)
