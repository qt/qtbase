CONFIG += testcase
TARGET = tst_qaccessibilitylinux
SOURCES += tst_qaccessibilitylinux.cpp \
    ../../../../src/platformsupport/linuxaccessibility/dbusconnection.cpp \
    ../../../../src/platformsupport/linuxaccessibility/struct_marshallers.cpp

CONFIG += gui
CONFIG += link_pkgconfig

QT += gui gui-private widgets dbus testlib

PKGCONFIG += atspi-2
