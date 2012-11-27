QT -= gui
QT += dbus
win32:CONFIG += console

SOURCES += listnames.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/dbus/listnames
INSTALLS += target

simulator: warning(This example does not work on Simulator platform)
