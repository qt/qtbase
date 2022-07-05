# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qabstractfileengine.pro.

#####################################################################
## tst_qabstractfileengine Test:
#####################################################################

qt_internal_add_test(tst_qabstractfileengine
    SOURCES
        tst_qabstractfileengine.cpp
    LIBRARIES
        Qt::CorePrivate
)

# Resources:
set(qabstractfileengine_resource_files
    "resources/"
)

qt_internal_add_resource(tst_qabstractfileengine "qabstractfileengine"
    PREFIX
        "/tst_qabstractfileengine/"
    FILES
        ${qabstractfileengine_resource_files}
)

