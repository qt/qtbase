# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# A header with Q_OBJECT whose moc is NOT #includ'ed by any source.
# AUTOMOC generates moc_foo.cpp in a checksum-named directory and compiles it via
# mocs_compilation.cpp.
add_library(lib1)
qt_extract_metatypes(lib1)
target_sources(lib1 PRIVATE mocs_compilation/foo.cpp)
target_link_libraries(lib1 PRIVATE Qt::Core)
