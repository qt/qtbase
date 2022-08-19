# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qdbusabstractadaptor.pro.

add_subdirectory(qmyserver)

# special case begin
qt_internal_add_test(tst_qdbusabstractadaptor
    SOURCES
        myobject.h
        tst_qdbusabstractadaptor.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::DBus
)
# special case end
add_dependencies(tst_qdbusabstractadaptor qmyserver)
