# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qdebug.pro.

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
