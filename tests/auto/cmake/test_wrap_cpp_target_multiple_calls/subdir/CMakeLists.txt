# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

target_sources(example PRIVATE
    myobject1.cpp
    myobject2.cpp
)
qt_wrap_cpp(example myobject1.h)
qt_wrap_cpp(example myobject2.h)
