# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# A source file that omits its foo.moc include, AUTOMOC fails.
add_library(lib1)
qt_extract_metatypes(lib1)
target_sources(lib1 PRIVATE dot_moc_missing/foo.cpp)
target_link_libraries(lib1 PRIVATE Qt::Core)
