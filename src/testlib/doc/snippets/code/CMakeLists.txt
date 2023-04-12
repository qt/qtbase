# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_library(testlib_code_snippets OBJECT
    doc_src_qtqskip.cpp
    src_corelib_kernel_qtestsupport_core.cpp
)

target_link_libraries(testlib_code_snippets PRIVATE
    Qt::Core
    Qt::Test
)

qt_internal_extend_target(testlib_code_snippets CONDITION QT_FEATURE_gui AND QT_FEATURE_sql
    SOURCES
        doc_src_qtestlib.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Sql
)

qt_internal_extend_target(testlib_code_snippets CONDITION QT_FEATURE_widgets
    SOURCES
        doc_src_qtestevent.cpp
        doc_src_qtestevent.h
    PUBLIC_LIBRARIES
        Qt::Widgets
)
