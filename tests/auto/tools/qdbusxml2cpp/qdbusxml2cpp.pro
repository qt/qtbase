CONFIG += testcase
QT = core testlib
TARGET = tst_qdbusxml2cpp
SOURCES += tst_qdbusxml2cpp.cpp
QMAKE_CXXFLAGS += $$QT_CFLAGS_DBUS
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
