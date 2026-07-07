# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Two same-named pairs in sibling subdirectories.

# AUTOMOC needs the project source directory on the moc include search path to
# locate the siblings/*/foo.h headers.
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(lib1)
qt_extract_metatypes(lib1)
target_sources(lib1 PRIVATE siblings/a/foo.cpp siblings/b/foo.cpp)
target_link_libraries(lib1 PRIVATE Qt::Core)
