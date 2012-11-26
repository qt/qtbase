QT -= gui
QT += dbus

HEADERS += ping-common.h
SOURCES += ping.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/dbus/pingpong
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/dbus/pingpong
INSTALLS += target sources

simulator: warning(This example does not work on Simulator platform)
