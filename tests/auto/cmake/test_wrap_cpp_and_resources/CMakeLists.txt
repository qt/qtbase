# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


cmake_minimum_required(VERSION 3.16)

project(test_wrap_cpp_and_resources)

find_package(Qt6Core REQUIRED)

qt_wrap_cpp(moc_files myobject.h)

qt_add_resources(rcc_files "test_wrap_cpp_and_resources.qrc")

add_executable(myobject myobject.cpp ${moc_files} ${rcc_files})
target_link_libraries(myobject PRIVATE Qt::Core)
