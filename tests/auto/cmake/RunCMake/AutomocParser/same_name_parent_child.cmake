# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Two same-named pairs in a parent and a child directory.

# AUTOMOC needs the project source directory on the moc include search path to
# locate the sub/foo.h header in the sub/ dir.
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(lib1)
qt_extract_metatypes(lib1)
target_sources(lib1 PRIVATE foo.cpp)
target_link_libraries(lib1 PRIVATE Qt::Core)
add_subdirectory(sub)
