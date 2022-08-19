# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


cmake_minimum_required(VERSION 3.16)

project(test_qtmainwin_library)

find_package(Qt6Core REQUIRED)

qt_wrap_cpp(moc_files myobject.h)

# On non-windows, the WIN32 is harmless, and Qt6Core_QTMAIN_LIBRARIES is empty.
# We test that it is harmless on those, and test that it builds on Windows.
# It wouldn't build if WIN32 is used and Qt6Core_QTMAIN_LIBRARIES is empty.
add_executable(myobject WIN32 myobject.cpp ${moc_files} )
target_link_libraries(myobject PRIVATE Qt::Core ${Qt6Core_QTMAIN_LIBRARIES})
