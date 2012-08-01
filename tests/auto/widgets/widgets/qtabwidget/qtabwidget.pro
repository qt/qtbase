CONFIG += testcase
TARGET = tst_qtabwidget

QT += widgets widgets-private testlib

INCLUDEPATH += ../

HEADERS +=  
SOURCES += tst_qtabwidget.cpp

win32:!wince*:LIBS += -luser32
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
