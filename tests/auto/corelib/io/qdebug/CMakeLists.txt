# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
    set_source_files_properties(tst_qdebug.cpp PROPERTIES LANGUAGE OBJCXX)
endif()
