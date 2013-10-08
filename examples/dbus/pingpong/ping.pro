QT -= gui
QT += dbus

HEADERS += ping-common.h
SOURCES += ping.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/dbus/pingpong
INSTALLS += target
