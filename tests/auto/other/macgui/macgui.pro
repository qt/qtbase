CONFIG += testcase
TARGET = tst_macgui

SOURCES += tst_macgui.cpp guitest.cpp
HEADERS += guitest.h

QT = core-private widgets-private testlib

requires(mac)

CONFIG+=insignificant_test  # QTBUG-20984, fails unstably
