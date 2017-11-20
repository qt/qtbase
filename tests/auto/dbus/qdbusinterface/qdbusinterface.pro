CONFIG += testcase
TARGET = tst_qdbusinterface
QT = core testlib
TEMPLATE = subdirs
qdbusinterface.depends = qmyserver
SUBDIRS = qmyserver qdbusinterface
