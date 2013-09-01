CONFIG += testcase
TARGET = tst_qtabwidget

QT += widgets widgets-private testlib

INCLUDEPATH += ../

HEADERS +=  
SOURCES += tst_qtabwidget.cpp

win32:!wince*:!winrt:LIBS += -luser32
