load(qttest_p4)
QT = core core-private
contains(QT_CONFIG,dbus): {
	SOURCES += tst_qdbusxmlparser.cpp
	QT += dbus dbus-private
} else {
	SOURCES += ../qdbusmarshall/dummy.cpp
}


