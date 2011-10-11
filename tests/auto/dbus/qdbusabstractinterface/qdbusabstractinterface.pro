load(qttest_p4)
TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = qpinger test
OTHER_FILES += com.trolltech.QtDBus.Pinger.xml
