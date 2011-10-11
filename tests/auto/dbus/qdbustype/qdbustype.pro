load(qttest_p4)
QT = core-private dbus-private
SOURCES += tst_qdbustype.cpp
QMAKE_CXXFLAGS += $$QT_CFLAGS_DBUS
LIBS_PRIVATE += $$QT_LIBS_DBUS
