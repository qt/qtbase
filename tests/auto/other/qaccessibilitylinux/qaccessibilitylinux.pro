CONFIG += testcase

include($$QT_SOURCE_TREE/src/platformsupport/accessibility/accessibility.pri)
include($$QT_SOURCE_TREE/src/platformsupport/linuxaccessibility/linuxaccessibility.pri)

TARGET = tst_qaccessibilitylinux
SOURCES += tst_qaccessibilitylinux.cpp

CONFIG += gui

QT += gui-private widgets dbus testlib

