# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdiriterator Test:
#####################################################################

# Collect test data
list(APPEND test_data "entrylist")

qt_internal_add_test(tst_qdiriterator
    SOURCES
        tst_qdiriterator.cpp
    LIBRARIES
        Qt::CorePrivate
    TESTDATA ${test_data}
)

# Resources:
set(qdiriterator_resource_files
    "entrylist/directory/dummy"
    "entrylist/file"
)

qt_internal_add_resource(tst_qdiriterator "qdiriterator"
    PREFIX
        "/testdata/"
    FILES
        ${qdiriterator_resource_files}
)


## Scopes:
#####################################################################

qt_internal_extend_target(tst_qdiriterator CONDITION CONFIG___contains___builtin_testdata
    DEFINES
        BUILTIN_TESTDATA
)
