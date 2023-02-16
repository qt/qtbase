# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_qdbusabstractinterface
    SOURCES
        interface.cpp
        tst_qdbusabstractinterface.cpp
    LIBRARIES
        Qt::Core
        Qt::DBus
)

qt_internal_extend_target(tst_qdbusabstractinterface
    DBUS_INTERFACE_SOURCES
        org.qtproject.QtDBus.Pinger.xml
    DBUS_INTERFACE_BASENAME
        pinger_interface
    DBUS_INTERFACE_FLAGS
        -i interface.h
)

add_subdirectory(qpinger)
add_dependencies(tst_qdbusabstractinterface qpinger)
