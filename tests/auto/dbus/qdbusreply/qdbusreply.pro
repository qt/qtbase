load(qttest_p4)
QT = core
contains(QT_CONFIG,dbus): {
	SOURCES += tst_qdbusreply.cpp
	QT += dbus
} else {
	SOURCES += ../qdbusmarshall/dummy.cpp
}


