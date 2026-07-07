# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Two same-named headers (foo.h and sub/foo.h), each with Q_OBJECT and neither
# moc #include'd by a source, so both moc files are added to mocs_compilation.cpp.
add_library(lib1)
qt_extract_metatypes(lib1)
target_sources(lib1 PRIVATE
    mocs_compilation_same_name/foo.cpp
    mocs_compilation_same_name/sub/foo.cpp
)
target_link_libraries(lib1 PRIVATE Qt::Core)
