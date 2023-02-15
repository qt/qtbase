# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdebug Test:
#####################################################################

qt_internal_add_test(tst_qdebug
    SOURCES
        tst_qdebug.cpp
    LIBRARIES
        Qt::Concurrent
)

if (APPLE)
    target_compile_options(tst_qdebug PRIVATE -x objective-c++)
endif()
