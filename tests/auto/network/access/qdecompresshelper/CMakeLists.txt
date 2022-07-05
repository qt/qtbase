# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qdecompresshelper.pro.

#####################################################################
## tst_qdecompresshelper Test:
#####################################################################

qt_internal_add_test(tst_qdecompresshelper
    SOURCES
        gzip.rcc.cpp
        inflate.rcc.cpp
        tst_qdecompresshelper.cpp
        zstandard.rcc.cpp
    DEFINES
        SRC_DIR=${CMAKE_CURRENT_SOURCE_DIR} # special case
    LIBRARIES
        Qt::NetworkPrivate
)

#### Keys ignored in scope 1:.:.:qdecompresshelper.pro:<TRUE>:
# TEMPLATE = "app"
