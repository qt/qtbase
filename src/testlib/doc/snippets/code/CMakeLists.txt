# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

add_library(testlib_code_snippets OBJECT
    doc_src_qtqskip.cpp
    src_corelib_kernel_qtestsupport_core.cpp
)

target_link_libraries(testlib_code_snippets PRIVATE
    Qt::Core
    Qt::Test
)

if(QT_FEATURE_gui AND QT_FEATURE_sql)
    target_sources(testlib_code_snippets PRIVATE
        doc_src_qtestlib.cpp
    )
    target_link_libraries(testlib_code_snippets PUBLIC
        Qt::Gui
        Qt::Sql
    )
endif()

if(QT_FEATURE_widgets)
    target_sources(testlib_code_snippets PRIVATE
        doc_src_qtestevent.cpp
        doc_src_qtestevent.h
    )
    target_link_libraries(testlib_code_snippets PUBLIC
        Qt::Widgets
    )
endif()
