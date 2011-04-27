load(qttest_p4)
QT = core
contains(QT_CONFIG,dbus): {
    TEMPLATE = subdirs
    CONFIG += ordered
    SUBDIRS = qmyserver test
} else {
    SOURCES += ../qdbusmarshall/dummy.cpp
}


