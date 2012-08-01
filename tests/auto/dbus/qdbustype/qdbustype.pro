CONFIG += testcase
TARGET = tst_qdbustype
QT = core-private dbus-private testlib
SOURCES += tst_qdbustype.cpp
QMAKE_CXXFLAGS += $$QT_CFLAGS_DBUS
LIBS_PRIVATE += $$QT_LIBS_DBUS
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
