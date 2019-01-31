CONFIG += testcase
TARGET = tst_macgui

SOURCES += tst_macgui.cpp guitest.cpp
HEADERS += guitest.h

QT = core-private widgets-private testlib

osx: LIBS += -framework ApplicationServices

requires(mac)
requires(widgets)
