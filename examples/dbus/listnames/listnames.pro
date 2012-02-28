QT -= gui
CONFIG += qdbus
win32:CONFIG += console

SOURCES += listnames.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dbus/listnames
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dbus/listnames
INSTALLS += target sources

simulator: warning(This example does not work on Simulator platform)
