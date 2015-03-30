CONFIG += testcase
TARGET = tst_qdbusabstractinterface
TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = qpinger qdbusabstractinterface
OTHER_FILES += org.qtproject.QtDBus.Pinger.xml
