CONFIG += testcase
TARGET = tst_qdbusmarshall
TEMPLATE = subdirs
qdbusmarshall.depends = qpong
SUBDIRS = qpong qdbusmarshall

QT = core-private testlib

requires(qtConfig(private_tests))
