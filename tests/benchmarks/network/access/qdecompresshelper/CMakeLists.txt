# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qdecompresshelper.pro.

#####################################################################
## qdecompresshelper Binary:
#####################################################################

qt_internal_add_benchmark(qdecompresshelper
    SOURCES
        main.cpp
    DEFINES
        SRC_DIR=${CMAKE_CURRENT_SOURCE_DIR} # special case
    LIBRARIES
        Qt::NetworkPrivate
        Qt::Test
)

#### Keys ignored in scope 1:.:.:decompresshelper.pro:<TRUE>:
# TEMPLATE = "app"
# _REQUIREMENTS = "qtConfig(private_tests)"
