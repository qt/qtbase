load(qttest_p4)
SOURCES += ../tst_qdbusabstractinterface.cpp ../interface.cpp
HEADERS += ../interface.h

# These are generated sources
# To regenerate, see the command-line at the top of the files
SOURCES += ../pinger.cpp
HEADERS += ../pinger.h

TARGET = ../tst_qdbusabstractinterface

QT = core
QT += dbus
CONFIG += insignificant_test #See QTBUG-21424
