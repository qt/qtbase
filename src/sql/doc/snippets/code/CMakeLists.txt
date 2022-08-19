# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_library(code_snippets OBJECT
    doc_src_sql-driver.cpp
    src_sql_kernel_qsqldatabase.cpp
    src_sql_kernel_qsqlerror.cpp
    src_sql_kernel_qsqlresult.cpp
    src_sql_kernel_qsqldriver.cpp
    src_sql_models_qsqlquerymodel.cpp
)

target_link_libraries(code_snippets PRIVATE
    Qt::Core
    Qt::Sql
)
