# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

add_executable(nospace main.cpp)

add_executable(onespace main.cpp)
set_target_properties(onespace PROPERTIES OUTPUT_NAME "one space")

add_executable(twospaces main.cpp)
set_target_properties(twospaces PROPERTIES OUTPUT_NAME "two space s")
