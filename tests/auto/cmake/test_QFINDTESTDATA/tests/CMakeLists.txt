# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


add_executable(test_QFINDTESTDATA WIN32 main.cpp)

target_link_libraries(test_QFINDTESTDATA Qt::Test)
