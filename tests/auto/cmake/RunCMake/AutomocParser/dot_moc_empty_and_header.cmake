# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# A source that has no Q_OBJECT and #include's an (empty) <base>.moc
add_library(lib1)
qt_extract_metatypes(lib1)
target_sources(lib1 PRIVATE dot_moc_empty_and_header/foo.cpp)
target_link_libraries(lib1 PRIVATE Qt::Core)
