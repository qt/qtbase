QT -= gui
QT += dbus

HEADERS += complexping.h ping-common.h
SOURCES += complexping.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/dbus/complexpingpong
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/dbus/complexpingpong
INSTALLS += target sources

simulator: warning(This example does not work on Simulator platform)
