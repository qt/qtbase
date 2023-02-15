# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qcborstreamwriter Test:
#####################################################################

qt_internal_add_test(tst_qcborstreamwriter
    SOURCES
        tst_qcborstreamwriter.cpp
    INCLUDE_DIRECTORIES
        ../../../../../src/3rdparty/tinycbor/tests/encoder
)
