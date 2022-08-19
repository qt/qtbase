# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qstringtokenizer Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qstringtokenizer
    SOURCES
        tst_bench_qstringtokenizer.cpp
    LIBRARIES
        Qt::Test
)

qt_add_resources(tst_bench_qstringtokenizer "qstringtokenizer_bench_data"
    PREFIX
        "/"
    FILES
        "./data/lorem.txt"
)
