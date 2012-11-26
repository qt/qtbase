QT -= gui
QT += dbus

HEADERS += complexpong.h
SOURCES += complexpong.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/dbus/complexpingpong
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/dbus/complexpingpong
INSTALLS += target sources

simulator: warning(This example does not work on Simulator platform)
