CONFIG += testcase
TARGET = tst_qdbusabstractinterface
TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = qpinger test
OTHER_FILES += com.trolltech.QtDBus.Pinger.xml
