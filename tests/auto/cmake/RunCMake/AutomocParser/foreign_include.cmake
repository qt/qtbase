# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# A source that #include's the moc of a foreign header:
# consumer/bar.cpp includes "sub/moc_foo.cpp", but the matching sub/foo.h lives under the lib
# subdirectory.
add_library(lib1)
qt_extract_metatypes(lib1)
target_sources(lib1 PRIVATE
    foreign_include/consumer/bar.cpp
    foreign_include/lib/sub/foo.h
)
# Put foreign_include/lib on the moc include search path
target_include_directories(lib1 PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/foreign_include/lib")
target_link_libraries(lib1 PRIVATE Qt::Core)
