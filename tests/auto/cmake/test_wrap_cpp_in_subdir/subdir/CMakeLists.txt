# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(source_files myobject.cpp)
qt_wrap_cpp(source_files TARGET example
    myobject.h
)
target_sources(example PRIVATE ${source_files})
