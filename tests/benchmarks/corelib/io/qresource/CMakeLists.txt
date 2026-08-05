# Copyright (C) 2026 Aurélien Brooke <aurelien@bahiasoft.fr>
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_benchmark(tst_bench_qresource
    SOURCES
        tst_bench_qresource.cpp
    LIBRARIES
        Qt::Test
)

qt_add_binary_resources(tst_bench_qresource_resource "resource.qrc"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/resource.rcc"
    OPTIONS -binary
)
add_dependencies(tst_bench_qresource tst_bench_qresource_resource)
