CONFIG += testcase
TARGET = tst_qdbusabstractadaptor
QT = core core-private testlib
TEMPLATE = subdirs

qdbusabstractadaptor.depends = qmyserver
SUBDIRS = qmyserver qdbusabstractadaptor
