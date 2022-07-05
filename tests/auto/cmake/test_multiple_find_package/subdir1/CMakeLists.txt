# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


find_package(Qt6Core REQUIRED)

add_executable(exe2 "${CMAKE_CURRENT_SOURCE_DIR}/../main.cpp")
target_link_libraries(exe2 Qt::Core)
