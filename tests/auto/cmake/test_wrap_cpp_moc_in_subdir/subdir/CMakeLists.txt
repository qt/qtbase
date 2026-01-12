# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(source_files myobject.cpp)
set(orig_source_files ${source_files})
qt_wrap_cpp(source_files
    TARGET example
    myobject.cpp
)
target_sources(example PRIVATE ${source_files})
if(NOT source_files STREQUAL orig_source_files)
    message("test_wrap_moc_in_subdir: source_files was unexpectedly changed to ${source_files}")
endif()
