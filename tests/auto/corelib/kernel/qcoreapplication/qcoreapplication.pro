CONFIG += testcase
TARGET = tst_qcoreapplication
QT = core testlib core-private
SOURCES = tst_qcoreapplication.cpp
HEADERS = tst_qcoreapplication.h
requires(qtConfig(private_tests))
