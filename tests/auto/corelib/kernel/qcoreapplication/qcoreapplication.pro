CONFIG += testcase
TARGET = tst_qcoreapplication
QT = core testlib core-private
SOURCES = tst_qcoreapplication.cpp
HEADERS = tst_qcoreapplication.h
requires(contains(QT_CONFIG,private_tests))
