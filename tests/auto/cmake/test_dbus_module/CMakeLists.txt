# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


cmake_minimum_required(VERSION 3.16)

project(test_dbus_module)

find_package(Qt6DBus 6.0.0 REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(my_srcs mydbusobject.cpp)

qt_wrap_cpp(moc_files mydbusobject.h)

qt_generate_dbus_interface(
    mydbusobject.h
    ${CMAKE_BINARY_DIR}/org.qtProject.Tests.MyDBusObject.xml
)

qt_add_dbus_adaptor(my_srcs
    ${CMAKE_BINARY_DIR}/org.qtProject.Tests.MyDBusObject.xml
    mydbusobject.h
    MyDBusObject
)

add_executable(myobject ${my_srcs} ${moc_files})
target_link_libraries(myobject PRIVATE Qt6::DBus)
