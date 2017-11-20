CONFIG += testcase
TARGET = tst_qdbusabstractinterface
TEMPLATE = subdirs
qdbusabstractinterface.depends = qpinger
SUBDIRS = qpinger qdbusabstractinterface
OTHER_FILES += org.qtproject.QtDBus.Pinger.xml
