# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# A single source/header pair in a subdir.
add_library(lib1)
qt_extract_metatypes(lib1)
target_sources(lib1 PRIVATE single/foo.cpp)
target_link_libraries(lib1 PRIVATE Qt::Core)
