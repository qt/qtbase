load(qttest_p4)
QT = core
contains(QT_CONFIG,dbus): {
	SOURCES += tst_qdbusmetaobject.cpp
	QT += dbus dbus-private
} else {
	SOURCES += ../qdbusmarshall/dummy.cpp
}


