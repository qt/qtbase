# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qlist Test:
#####################################################################

qt_internal_add_test(tst_qlist
    SOURCES
        tst_qlist.cpp
    DEFINES
        QT_NO_STRICT_QLIST_ITERATORS # doesn't exist, except for this test
    LIBRARIES
        Qt::CorePrivate
)

qt_internal_add_test(tst_qlist_strict_iterators
    SOURCES
        tst_qlist.cpp
    DEFINES
        QT_STRICT_QLIST_ITERATORS
        tst_QList=tst_QList_strict_iterators
    LIBRARIES
        Qt::CorePrivate
)

## Scopes:
#####################################################################
