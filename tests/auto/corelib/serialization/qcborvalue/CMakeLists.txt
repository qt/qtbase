# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qcborvalue Test:
#####################################################################

qt_internal_add_test(tst_qcborvalue
    SOURCES
        tst_qcborvalue.cpp
    INCLUDE_DIRECTORIES
        ../../../../../src/3rdparty/tinycbor/src
        ../../../../../src/3rdparty/tinycbor/tests/parser
    LIBRARIES
        Qt::CorePrivate
)
