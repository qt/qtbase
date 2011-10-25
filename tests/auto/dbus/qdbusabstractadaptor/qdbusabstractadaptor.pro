CONFIG += testcase
TARGET = tst_qdbusabstractadaptor
QT = core core-private testlib
TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = qmyserver test
