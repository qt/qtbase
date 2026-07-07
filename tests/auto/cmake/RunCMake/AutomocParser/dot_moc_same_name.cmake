# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Two same-named source files each #include's its own .moc.
add_library(lib1)
qt_extract_metatypes(lib1)
target_sources(lib1 PRIVATE dot_moc_same_name/foo.cpp)
target_link_libraries(lib1 PRIVATE Qt::Core)
add_subdirectory(dot_moc_same_name/sub)
