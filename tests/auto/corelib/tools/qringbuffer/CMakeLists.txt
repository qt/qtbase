# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qringbuffer Test:
#####################################################################

qt_internal_add_test(tst_qringbuffer
    SOURCES
        tst_qringbuffer.cpp
    LIBRARIES
        Qt::CorePrivate
)
