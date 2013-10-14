QT -= gui
QT += dbus

HEADERS += complexping.h ping-common.h
SOURCES += complexping.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/dbus/complexpingpong
INSTALLS += target
