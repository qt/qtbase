# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qaccessibilitylinux LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

if (NOT QT_FEATURE_accessibility OR NOT TARGET Qt::LinuxAccessibilitySupportPrivate
        OR (QT_BUILD_STANDALONE_TESTS AND QT_WILL_INSTALL))
    return()
endif()

#####################################################################
## tst_qaccessibilitylinux Test:
#####################################################################

qt_internal_add_test(tst_qaccessibilitylinux
    SOURCES
        tst_qaccessibilitylinux.cpp
    DBUS_INTERFACE_SOURCES
        ../../../../src/platformsupport/linuxaccessibility/dbusxml/Bus.xml
    LIBRARIES
        Qt::DBus
        Qt::Gui
        Qt::GuiPrivate
        Qt::LinuxAccessibilitySupportPrivate
        Qt::Widgets
)

# require for struct_marshallers_p.h which is included dbus_interface.h
target_include_directories(tst_qaccessibilitylinux PRIVATE
    ../../../../src/platformsupport/linuxaccessibility
)
