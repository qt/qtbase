load(qttest_p4)
QT = core core-private
contains(QT_CONFIG,dbus): {
        SOURCES += tst_qdbustype.cpp
        QT += dbus dbus-private
        QMAKE_CXXFLAGS += $$QT_CFLAGS_DBUS
        LIBS_PRIVATE += $$QT_LIBS_DBUS
} else {
        SOURCES += ../qdbusmarshall/dummy.cpp
}
