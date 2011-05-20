load(qttest_p4)
SOURCES += ../tst_qdbusmarshall.cpp
TARGET = ../tst_qdbusmarshall

QT = core
QT += core-private dbus-private

LIBS += $$QT_LIBS_DBUS
QMAKE_CXXFLAGS += $$QT_CFLAGS_DBUS
