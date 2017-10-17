CONFIG += testcase
TARGET = tst_qcoreapplication
QT = core testlib core-private
SOURCES = tst_qcoreapplication.cpp
HEADERS = tst_qcoreapplication.h
win32: VERSION = 1.2.3.4
else: VERSION = 1.2.3
QMAKE_INFO_PLIST = $$PWD/Info.plist
requires(qtConfig(private_tests))
