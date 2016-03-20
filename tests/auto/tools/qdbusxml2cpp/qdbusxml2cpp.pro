CONFIG += testcase
QT = core testlib
TARGET = tst_qdbusxml2cpp
SOURCES += tst_qdbusxml2cpp.cpp
QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_DBUS
