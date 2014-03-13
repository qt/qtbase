CONFIG += testcase
SOURCES += ../tst_qdbusmarshall.cpp
TARGET = ../tst_qdbusmarshall

QT = core-private dbus-private testlib

LIBS += $$QT_LIBS_DBUS
QMAKE_CXXFLAGS += $$QT_CFLAGS_DBUS
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

macx:CONFIG += insignificant_test # QTBUG-37469
