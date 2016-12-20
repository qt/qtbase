CONFIG += testcase
TARGET = tst_qcoreapplication
QT = core testlib core-private
SOURCES = tst_qcoreapplication.cpp
HEADERS = tst_qcoreapplication.h
win32: VERSION = 1.2.3.4
else: VERSION = 1.2.3
darwin: QMAKE_LFLAGS += -Wl,-sectcreate,__TEXT,__info_plist,$$shell_quote($$PWD/Info.plist)
requires(qtConfig(private_tests))
