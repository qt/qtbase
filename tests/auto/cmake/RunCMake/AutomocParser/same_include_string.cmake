# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Two same-named sources that both #include the *bare* "moc_foo.cpp"
# (instead of the disambiguating "sub/moc_foo.cpp").
# The two includes could resolve to different headers, so AUTOMOC rejects this
# at build time and asks for a directory prefix.
add_library(lib1)
qt_extract_metatypes(lib1)
target_sources(lib1 PRIVATE same_include_string/foo.cpp)
target_link_libraries(lib1 PRIVATE Qt::Core)
add_subdirectory(same_include_string/sub)
