# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(source_files
    myobject1.cpp
    myobject2.cpp
)
qt_wrap_cpp(source_files TARGET example
    myobject1.h
)
qt_wrap_cpp(source_files TARGET example
    myobject2.h
)
target_sources(example PRIVATE ${source_files})
