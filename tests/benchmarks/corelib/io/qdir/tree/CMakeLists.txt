# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qdir_tree Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qdir_tree
    SOURCES
        tst_bench_qdir_tree.cpp
    LIBRARIES
        Qt::Test
)

# Resources:
set(qdir_tree_resource_files
    "4.6.0-list.txt"
)

qt_internal_add_resource(tst_bench_qdir_tree "tst_bench_qdir_tree"
    PREFIX
        "/"
    FILES
        ${qdir_tree_resource_files}
)
