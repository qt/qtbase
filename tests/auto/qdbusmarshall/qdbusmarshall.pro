load(qttest_p4)
contains(QT_CONFIG,dbus): {
	TEMPLATE = subdirs
	CONFIG += ordered
	SUBDIRS = qpong test

        requires(contains(QT_CONFIG,private_tests))
} else {
	SOURCES += dummy.cpp
}


