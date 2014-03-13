CONFIG += testcase
TARGET = tst_qdbuslocalcalls
QT = core dbus testlib
SOURCES += tst_qdbuslocalcalls.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

macx:CONFIG += insignificant_test # QTBUG-37469
