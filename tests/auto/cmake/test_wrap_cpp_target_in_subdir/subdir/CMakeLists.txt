# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_wrap_cpp(example myobject.h)
target_sources(example PRIVATE myobject.cpp)
