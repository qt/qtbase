load(qttest_p4)
contains(QT_CONFIG,dbus): {
    TEMPLATE = subdirs
    CONFIG += ordered
    SUBDIRS = qpinger test
} else {
    SOURCES += ../qdbusmarshall/dummy.cpp
}

OTHER_FILES += com.trolltech.QtDBus.Pinger.xml
