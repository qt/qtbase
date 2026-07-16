# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Like same_name_parent_child, but each source also declares its own Q_OBJECT
# and #include's its own .moc.

# AUTOMOC needs the project source directory on the moc include search path to
# locate the sub_bar/bar.h header.
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(lib1)
qt_extract_metatypes(lib1)
target_sources(lib1 PRIVATE bar.cpp)
target_link_libraries(lib1 PRIVATE Qt::Core)
add_subdirectory(sub_bar)
