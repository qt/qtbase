# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#! [qt_wrap_ui]
set(SOURCES mainwindow.cpp main.cpp)
qt_wrap_ui(SOURCES mainwindow.ui)
qt_add_executable(myapp ${SOURCES})
#! [qt_wrap_ui]
