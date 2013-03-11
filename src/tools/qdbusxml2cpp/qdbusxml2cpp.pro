option(host_build)
QT += bootstrap_dbus-private
DEFINES += QT_NO_CAST_FROM_ASCII
QMAKE_CXXFLAGS += $$QT_CFLAGS_DBUS

SOURCES = qdbusxml2cpp.cpp

load(qt_tool)
