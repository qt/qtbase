# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qimageconversion.pro.

#####################################################################
## tst_bench_imageConversion Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_imageConversion
    SOURCES
        tst_qimageconversion.cpp
    LIBRARIES
        Qt::Gui
        Qt::Test
)

#### Keys ignored in scope 1:.:.:qimageconversion.pro:<TRUE>:
# QT_FOR_CONFIG = "gui-private"
# TEMPLATE = "app"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_bench_imageConversion CONDITION QT_FEATURE_gif
    DEFINES
        QTEST_HAVE_GIF
)

qt_internal_extend_target(tst_bench_imageConversion CONDITION QT_FEATURE_jpeg
    DEFINES
        QTEST_HAVE_JPEG
)
