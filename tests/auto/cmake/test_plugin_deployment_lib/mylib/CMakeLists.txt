# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

find_package(Qt6 REQUIRED COMPONENTS Gui)
qt_add_library(MyLib SHARED mylib.cpp)
target_compile_definitions(MyLib PRIVATE BUILD_MYLIB)
target_link_libraries(MyLib PRIVATE Qt6::Gui)
