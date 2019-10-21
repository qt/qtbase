TARGET = QtLinuxOfonoSupport
MODULE = linuxofono_support

QT = core dbus
CONFIG += static internal_module

DEFINES += QT_NO_CAST_FROM_ASCII

HEADERS += \
    qofonoservice_linux_p.h

SOURCES += \
    qofonoservice_linux.cpp

load(qt_module)
