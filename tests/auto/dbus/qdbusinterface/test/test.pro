CONFIG += testcase
SOURCES += ../tst_qdbusinterface.cpp
HEADERS += ../myobject.h
TARGET = ../tst_qdbusinterface

QT = core core-private dbus testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

macx:CONFIG += insignificant_test # QTBUG-37469
