CONFIG += testcase

# This is temporary to start running the test as part of normal CI.
CONFIG += insignificant_test # QTBUG-27732

include($$QT_SOURCE_TREE/src/platformsupport/linuxaccessibility/linuxaccessibility.pri)

TARGET = tst_qaccessibilitylinux
SOURCES += tst_qaccessibilitylinux.cpp

CONFIG += gui

QT += gui-private widgets dbus testlib

