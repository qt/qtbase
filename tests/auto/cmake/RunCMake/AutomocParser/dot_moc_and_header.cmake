# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# A source that #include's its own <base>.moc (for a Q_OBJECT declared in the .cpp), next to a
# <base>.h header that also has a Q_OBJECT.
add_library(lib1)
qt_extract_metatypes(lib1)
target_sources(lib1 PRIVATE dot_moc_and_header/foo.cpp)
target_link_libraries(lib1 PRIVATE Qt::Core)
