# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qfilesystementry Test:
#####################################################################

qt_internal_add_test(tst_qfilesystementry
    SOURCES
        ../../../../../src/corelib/io/qfilesystementry.cpp
        ../../../../../src/corelib/io/qfilesystementry_p.h
        tst_qfilesystementry.cpp
    LIBRARIES
        Qt::CorePrivate
)
