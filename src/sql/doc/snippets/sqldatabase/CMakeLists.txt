# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_library(sqldatabase_snippets OBJECT
    sqldatabase.cpp
)

target_link_libraries(sqldatabase_snippets PRIVATE
    Qt::Core
    Qt::Sql
)
