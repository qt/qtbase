CONFIG += testcase
SOURCES += ../tst_qdbusabstractinterface.cpp ../interface.cpp
HEADERS += ../interface.h

# These are generated sources
# To regenerate, see the command-line at the top of the files
SOURCES += ../pinger.cpp
HEADERS += ../pinger.h

TARGET = ../tst_qdbusabstractinterface

QT = core testlib
QT += dbus
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
