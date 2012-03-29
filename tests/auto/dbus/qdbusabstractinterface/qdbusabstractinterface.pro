CONFIG += testcase
TARGET = tst_qdbusabstractinterface
TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = qpinger test
OTHER_FILES += org.qtproject.QtDBus.Pinger.xml
