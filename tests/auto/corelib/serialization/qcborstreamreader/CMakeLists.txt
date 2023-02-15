# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qcborstreamreader Test:
#####################################################################

qt_internal_add_test(tst_qcborstreamreader
    SOURCES
        tst_qcborstreamreader.cpp
    INCLUDE_DIRECTORIES
        ../../../../../src/3rdparty/tinycbor/src
        ../../../../../src/3rdparty/tinycbor/tests/parser
    LIBRARIES
        Qt::CorePrivate
)
