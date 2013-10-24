CONFIG += testcase
TARGET = tst_qdbusmarshall
TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = qpong test

QT = core-private testlib

requires(contains(QT_CONFIG,private_tests))
