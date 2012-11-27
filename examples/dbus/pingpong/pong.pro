QT -= gui
QT += dbus

HEADERS += ping-common.h pong.h
SOURCES += pong.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/dbus/pingpong
INSTALLS += target

simulator: warning(This example does not work on Simulator platform)
