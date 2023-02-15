# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qcollator Test:
#####################################################################

qt_internal_add_test(tst_qcollator
    SOURCES
        tst_qcollator.cpp
    DEFINES
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
)
