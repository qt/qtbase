CONFIG += testcase
TARGET = tst_noqteventloop

QT += core-private network gui-private testlib

SOURCES  += tst_noqteventloop.cpp

contains(QT_CONFIG,dynamicgl):win32:!wince*:!winrt: LIBS += -luser32
