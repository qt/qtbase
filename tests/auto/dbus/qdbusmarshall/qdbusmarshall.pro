CONFIG += testcase
TARGET = tst_qdbusmarshall
TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = qpong qdbusmarshall

QT = core-private testlib

requires(qtConfig(private_tests))
