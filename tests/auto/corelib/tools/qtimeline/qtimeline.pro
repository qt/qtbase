CONFIG += testcase parallel_test
TARGET = tst_qtimeline
QT = core testlib
SOURCES = tst_qtimeline.cpp
win32:CONFIG+=insignificant_test    # unstable, QTBUG-24796
