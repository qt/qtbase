CONFIG += testcase
TARGET = tst_qdbusinterface
QT = core testlib
TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = qmyserver test
