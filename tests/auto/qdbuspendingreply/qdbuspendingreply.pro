load(qttest_p4)
requires(contains(QT_CONFIG, dbus))
QT = core dbus
SOURCES += tst_qdbuspendingreply.cpp
