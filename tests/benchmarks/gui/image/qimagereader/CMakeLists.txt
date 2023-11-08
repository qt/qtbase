# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qimagereader Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qimagereader
    SOURCES
        tst_bench_qimagereader.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::Test
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_bench_qimagereader CONDITION QT_FEATURE_gif
    DEFINES
        QTEST_HAVE_GIF
)

qt_internal_extend_target(tst_bench_qimagereader CONDITION QT_FEATURE_jpeg
    DEFINES
        QTEST_HAVE_JPEG
)
