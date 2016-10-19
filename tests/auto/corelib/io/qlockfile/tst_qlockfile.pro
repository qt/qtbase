CONFIG += testcase
TARGET = tst_qlockfile
SOURCES += tst_qlockfile.cpp

QT = core testlib concurrent
win32:!winrt:LIBS += -ladvapi32
